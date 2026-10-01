#include "backend/opencl/runners/OclXelisHashRunner.h"

#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

#include "backend/opencl/OclLaunchData.h"
#include "backend/opencl/wrappers/OclError.h"
#include "backend/opencl/wrappers/OclLib.h"
#include "base/net/stratum/Job.h"

namespace xmrig {

namespace {
constexpr size_t kScratchWords = 531 * 128;
constexpr size_t kScratchBytes = kScratchWords * sizeof(uint64_t);
constexpr uint32_t kMaxSolutions = 1024;

void arg(cl_kernel k, cl_uint i, size_t n, const void *p)
{
    const cl_int ret = OclLib::setKernelArg(k, i, n, p);
    if (ret != CL_SUCCESS) {
        throw std::runtime_error(OclError::toString(ret));
    }
}

}

OclXelisHashRunner::OclXelisHashRunner(size_t index, const OclLaunchData &data) : OclBaseRunner(index, data)
{
    // Each Xelis v3 hash is independent. Use the RDNA wave/work-group size
    // supplied by the OpenCL profile and keep sixteen full groups in flight.
    // This hides the long random-memory stage much better than one-item
    // groups while using only a few hundred MiB on this 16 GiB card.
    m_workGroupSize = std::max(1u, data.thread.worksize());
    m_batch = m_workGroupSize * 16;
    // Keep the compiler from attempting enormous loop unrolling and allow
    // the AMD driver to fuse the integer/FP operations used by v3.
    m_options += " -cl-std=CL2.0 -cl-mad-enable -cl-no-signed-zeros"
                 " -cl-unsafe-math-optimizations -cl-fast-relaxed-math"
                 " -cl-finite-math-only";
}

OclXelisHashRunner::~OclXelisHashRunner()
{
    OclLib::release(m_kernel); OclLib::release(m_hashes); OclLib::release(m_scratch);
    OclLib::release(m_target); OclLib::release(m_solutions); OclLib::release(m_lut);
}

void OclXelisHashRunner::init()
{
    OclBaseRunner::init();
    m_hashes = OclLib::createBuffer(m_ctx, CL_MEM_READ_WRITE, m_batch * 32);
    m_scratch = OclLib::createBuffer(m_ctx, CL_MEM_READ_WRITE, static_cast<size_t>(m_batch) * kScratchBytes);
    m_target = OclLib::createBuffer(m_ctx, CL_MEM_READ_ONLY, 32);
    m_solutions = OclLib::createBuffer(m_ctx, CL_MEM_READ_WRITE, sizeof(uint64_t) * (1 + kMaxSolutions * 5));
    m_lut = OclLib::createBuffer(m_ctx, CL_MEM_READ_ONLY, sizeof(double) * 512);
}

void OclXelisHashRunner::build()
{
    OclBaseRunner::build();
    cl_int ret = CL_SUCCESS;
    m_kernel = OclLib::createKernel(m_program, "xelis_hash_v3_kernel", &ret);
    if (ret != CL_SUCCESS) throw std::runtime_error(std::string("Xelis enqueue: ") + OclError::toString(ret) + " (" + std::to_string(ret) + ")");
}

void OclXelisHashRunner::set(const Job &job, uint8_t *blob)
{
    uint8_t input[112] = {};
    std::memcpy(input, blob, std::min<size_t>(job.size(), sizeof(input)));
    enqueueWriteBuffer(m_input, CL_TRUE, 0, sizeof(input), input);

    uint8_t target[32] = {};
    // Miner builds do not expose the proxy-only raw target string. The
    // compact target is stored little-endian by Job, which is the format
    // consumed by the existing result verifier.
    const uint64_t compactTarget = job.target();
    std::memcpy(target, &compactTarget, sizeof(compactTarget));
    enqueueWriteBuffer(m_target, CL_TRUE, 0, sizeof(target), target);

    std::vector<double> lut(512);
    for (size_t i = 0; i < lut.size(); ++i) {
        const double center = static_cast<double>(512 + i + 0.5) * static_cast<double>(1u << 22);
        lut[i] = 1.0 / center;
    }
    enqueueWriteBuffer(m_lut, CL_TRUE, 0, lut.size() * sizeof(double), lut.data());

    arg(m_kernel, 0, sizeof(cl_mem), &m_input); arg(m_kernel, 1, sizeof(cl_mem), &m_hashes);
    arg(m_kernel, 2, sizeof(cl_mem), &m_scratch); arg(m_kernel, 4, sizeof(m_batch), &m_batch);
    arg(m_kernel, 5, sizeof(cl_mem), &m_target); arg(m_kernel, 6, sizeof(cl_mem), &m_solutions);
    arg(m_kernel, 7, sizeof(cl_mem), &m_lut);
}

void OclXelisHashRunner::run(uint32_t nonce, uint32_t, uint32_t *hashOutput)
{
    std::vector<uint64_t> zero(1 + kMaxSolutions * 5, 0);
    enqueueWriteBuffer(m_solutions, CL_FALSE, 0, zero.size() * sizeof(uint64_t), zero.data());
    const uint64_t start = nonce;
    arg(m_kernel, 3, sizeof(start), &start);

    const size_t global = m_batch;
    const size_t local = m_workGroupSize;
    const cl_int ret = OclLib::enqueueNDRangeKernel(m_queue, m_kernel, 1, nullptr, &global, &local, 0, nullptr, nullptr);
    if (ret != CL_SUCCESS) throw std::runtime_error(OclError::toString(ret));

    std::vector<uint64_t> solutions(zero.size());
    try {
        enqueueReadBuffer(m_solutions, CL_TRUE, 0, solutions.size() * sizeof(uint64_t), solutions.data());
    }
    catch (const std::exception &ex) {
        throw std::runtime_error(std::string("Xelis read: ") + ex.what());
    }
    const uint32_t count = std::min<uint32_t>(static_cast<uint32_t>(solutions[0]), 0xFF);
    hashOutput[0xFF] = count;
    for (uint32_t i = 0; i < count; ++i) hashOutput[i] = static_cast<uint32_t>(solutions[1 + i * 5]);
}

} // namespace xmrig
