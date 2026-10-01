#include "backend/opencl/runners/OclNexaPowRunner.h"

#include "backend/opencl/OclLaunchData.h"
#include "backend/opencl/wrappers/OclError.h"
#include "backend/opencl/wrappers/OclLib.h"
#include "base/net/stratum/Job.h"
#include "crypto/nexapow/NexaPow.h"
#include "nexapow_hip_cl.h"

#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

namespace xmrig {
namespace {
#if defined(_WIN32)
void hipCheck(hipError_t e, const char *what)
{
    if (e != hipSuccess) throw std::runtime_error(std::string(what) + ": " + hipGetErrorString(e));
}
void rtcCheck(hiprtcResult e, const char *what)
{
    if (e != HIPRTC_SUCCESS) throw std::runtime_error(std::string(what) + ": " + hiprtcGetErrorString(e));
}
#endif
void arg(cl_kernel kernel, cl_uint index, size_t size, const void *value)
{
    const cl_int ret = OclLib::setKernelArg(kernel, index, size, value);
    if (ret != CL_SUCCESS) throw std::runtime_error(OclError::toString(ret));
}
}

OclNexaPowRunner::OclNexaPowRunner(size_t index, const OclLaunchData &data) : OclBaseRunner(index, data)
{
    m_workGroupSize = std::max(1u, data.thread.worksize());
    // A larger batch amortizes OpenCL enqueue/readback overhead for the
    // register-heavy secp256k1 kernel on RDNA GPUs.
    m_batch = m_workGroupSize * 64;
    // The complete Nexa ECC path is now valid on the AMD JIT. Keep normal
    // optimization enabled; disabling it reduces the RX 9000 throughput by
    // orders of magnitude.
    m_options += " -cl-std=CL1.2";
}

OclNexaPowRunner::~OclNexaPowRunner()
{
#if defined(_WIN32)
    if (m_hipModule) hipModuleUnload(m_hipModule);
    if (m_hipRtc) hiprtcDestroyProgram(&m_hipRtc);
    if (m_hipHashes) hipFree(m_hipHashes);
    if (m_hipInput) hipFree(m_hipInput);
#endif
    OclLib::release(m_kernel);
    OclLib::release(m_hashes);
}

void OclNexaPowRunner::init()
{
    OclBaseRunner::init();
    m_hashes = OclLib::createBuffer(m_ctx, CL_MEM_READ_WRITE, static_cast<size_t>(m_batch) * 32);
#if defined(_WIN32)
    // HIP is used for NexaPoW on AMD; the OpenCL objects remain available as
    // a fallback for systems without ROCm.
    if (hipInit(0) == hipSuccess && hipSetDevice(static_cast<int>(data().device.index())) == hipSuccess) {
        hipCheck(hipMalloc(&m_hipInput, sizeof(m_blob)), "hipMalloc input");
        hipCheck(hipMalloc(&m_hipHashes, static_cast<size_t>(m_batch) * 32), "hipMalloc hashes");
    }
#endif
}

void OclNexaPowRunner::build()
{
#if defined(_WIN32)
    if (m_hipInput && m_hipHashes) {
        const char *opts[] = {"--offload-arch=gfx1200", "-O3"};
        rtcCheck(hiprtcCreateProgram(&m_hipRtc, nexapow_hip_cl, "nexapow_hip.hip", 0, nullptr, nullptr), "hiprtcCreateProgram");
        const hiprtcResult cr = hiprtcCompileProgram(m_hipRtc, 2, opts);
        if (cr != HIPRTC_SUCCESS) {
            size_t n = 0; hiprtcGetProgramLogSize(m_hipRtc, &n); std::string log(n, '\0');
            if (n) hiprtcGetProgramLog(m_hipRtc, &log[0]);
            throw std::runtime_error(std::string("HIPRTC NexaPoW: ") + log);
        }
        size_t codeSize = 0; rtcCheck(hiprtcGetCodeSize(m_hipRtc, &codeSize), "hiprtcGetCodeSize");
        std::vector<char> code(codeSize); rtcCheck(hiprtcGetCode(m_hipRtc, code.data()), "hiprtcGetCode");
        hipCheck(hipModuleLoadData(&m_hipModule, code.data()), "hipModuleLoadData");
        hipCheck(hipModuleGetFunction(&m_hipFunction, m_hipModule, "nexapow_sha_kernel"), "hipModuleGetFunction");
        m_hipReady = true;
        return;
    }
#endif
    OclBaseRunner::build();
    cl_int ret = CL_SUCCESS;
    m_kernel = OclLib::createKernel(m_program, "nexapow_sha_kernel", &ret);
    if (ret != CL_SUCCESS) throw std::runtime_error(std::string("NexaPoW kernel: ") + OclError::toString(ret));
}

void OclNexaPowRunner::set(const Job &job, uint8_t *blob)
{
    if (job.size() < sizeof(m_blob)) throw std::runtime_error("invalid NexaPoW work size");
    memcpy(m_blob, blob, sizeof(m_blob));
    m_target = job.target();
#if defined(_WIN32)
    if (m_hipReady) {
        hipCheck(hipMemcpyHtoD(m_hipInput, m_blob, sizeof(m_blob)), "hipMemcpyHtoD input");
        return;
    }
#endif
    enqueueWriteBuffer(m_input, CL_TRUE, 0, sizeof(m_blob), m_blob);
    arg(m_kernel, 0, sizeof(cl_mem), &m_input);
    arg(m_kernel, 1, sizeof(cl_mem), &m_hashes);
}

void OclNexaPowRunner::run(uint32_t nonce, uint32_t, uint32_t *hashOutput)
{
#if defined(_WIN32)
    if (m_hipReady) {
        const uint64_t start = nonce;
        void *args[] = { &m_hipInput, &m_hipHashes, const_cast<uint64_t *>(&start) };
        const unsigned blocks = (m_batch + m_workGroupSize - 1) / m_workGroupSize;
        hipCheck(hipModuleLaunchKernel(m_hipFunction, blocks, 1, 1, m_workGroupSize, 1, 1, 0, nullptr, args, nullptr), "hipModuleLaunchKernel");
        hipCheck(hipDeviceSynchronize(), "hipDeviceSynchronize");
        std::vector<uint8_t> miningHashes(static_cast<size_t>(m_batch) * 32);
        hipCheck(hipMemcpyDtoH(miningHashes.data(), m_hipHashes, miningHashes.size()), "hipMemcpyDtoH hashes");
        uint32_t count = 0;
        for (uint32_t i = 0; i < m_batch && count < 0xFF; ++i) {
            bool nonzero = false; for (uint32_t j = 0; j < 32; ++j) nonzero |= miningHashes[static_cast<size_t>(i) * 32 + j] != 0;
            if (nonzero && *reinterpret_cast<const uint64_t *>(miningHashes.data() + static_cast<size_t>(i) * 32 + 24) < m_target) hashOutput[count++] = start + i;
        }
        hashOutput[0xFF] = count;
        return;
    }
#endif
    const uint64_t start = nonce;
    arg(m_kernel, 2, sizeof(start), &start);
    const size_t global = m_batch;
    const size_t local = m_workGroupSize;
    const cl_int ret = OclLib::enqueueNDRangeKernel(m_queue, m_kernel, 1, nullptr, &global, &local, 0, nullptr, nullptr);
    if (ret != CL_SUCCESS) throw std::runtime_error(OclError::toString(ret));

    std::vector<uint8_t> miningHashes(static_cast<size_t>(m_batch) * 32);
    enqueueReadBuffer(m_hashes, CL_TRUE, 0, miningHashes.size(), miningHashes.data());

    uint32_t count = 0;
    for (uint32_t i = 0; i < m_batch && count < 0xFF; ++i) {
        bool nonzero = false;
        for (uint32_t j = 0; j < 32; ++j) nonzero |= miningHashes[static_cast<size_t>(i) * 32 + j] != 0;
        if (nonzero && *reinterpret_cast<const uint64_t *>(miningHashes.data() + static_cast<size_t>(i) * 32 + 24) < m_target) {
            hashOutput[count++] = static_cast<uint32_t>(start + i);
        }
    }
    hashOutput[0xFF] = count;
}
}
