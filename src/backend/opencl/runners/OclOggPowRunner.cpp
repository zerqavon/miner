#include <cstring>
#include <stdexcept>

#include "backend/opencl/runners/OclOggPowRunner.h"
#include "3rdparty/libethash/ethash_internal.h"
#include "backend/common/Tags.h"
#include "backend/opencl/OclLaunchData.h"
#include "backend/opencl/wrappers/OclError.h"
#include "backend/opencl/wrappers/OclLib.h"
#include "base/io/log/Log.h"
#include "base/io/log/Tags.h"
#include "base/net/stratum/Job.h"
#include "base/tools/Chrono.h"
#include "crypto/common/VirtualMemory.h"

#include "ProgPow.h"
#include "oggpow_cl.h"

namespace xmrig {

namespace {
constexpr uint32_t kMaxOutputs = 15;
constexpr uint32_t kOutputWords = 256;
constexpr uint32_t kResultWords = 16;
constexpr uint32_t kEpochLength = 30000;

static void setArg(cl_kernel kernel, cl_uint index, size_t size, const void *value)
{
    const cl_int ret = OclLib::setKernelArg(kernel, index, size, value);
    if (ret != CL_SUCCESS) {
        throw std::runtime_error(OclError::toString(ret));
    }
}
}

OclOggPowRunner::OclOggPowRunner(size_t index, const OclLaunchData &data) : OclBaseRunner(index, data)
{
    m_tunedIntensity = static_cast<uint32_t>(m_intensity);

    switch (data.thread.worksize()) {
    case 64: case 128: case 256: case 512:
        m_workGroupSize = data.thread.worksize();
        break;
    default:
        break;
    }

    if (data.device.vendorId() == OCL_VENDOR_NVIDIA) {
        m_options += " -DPLATFORM=OPENCL_PLATFORM_NVIDIA";
        m_dagWorkGroupSize = 32;
    }
    else if (data.device.vendorId() == OCL_VENDOR_AMD) {
        m_options += " -DPLATFORM=OPENCL_PLATFORM_AMD -cl-mad-enable -cl-no-signed-zeros";
        // The AMD compiler applies the required workgroup size used by the
        // search kernel to the program's DAG kernel as well.  Keep both
        // kernels on the same native workgroup size on RDNA devices.
        m_dagWorkGroupSize = m_workGroupSize;
    }
}

OclOggPowRunner::~OclOggPowRunner()
{
    OclLib::release(m_searchKernel);
    OclLib::release(m_dagKernel);
    OclLib::release(m_oggProgram);
    OclLib::release(m_lightCache);
    OclLib::release(m_dag);
    OclLib::release(m_stop);
}

void OclOggPowRunner::init()
{
    OclBaseRunner::init();
    m_stop = OclLib::createBuffer(m_ctx, CL_MEM_READ_WRITE, sizeof(uint32_t));
}

void OclOggPowRunner::build()
{
    // OggPoW's program is period-dependent and is compiled after the first
    // pool job supplies the block height.
}

void OclOggPowRunner::compileKernel(uint64_t period)
{
    OclLib::release(m_searchKernel);
    OclLib::release(m_dagKernel);
    OclLib::release(m_oggProgram);
    m_searchKernel = nullptr;
    m_dagKernel = nullptr;
    m_oggProgram = nullptr;

    const uint32_t epoch = static_cast<uint32_t>((period * 10) / kEpochLength);
    const uint32_t lightWords = static_cast<uint32_t>(KPCache::s_cache.size() / sizeof(node));
    const uint32_t dagItems = static_cast<uint32_t>(KPCache::dag_size(epoch) / sizeof(node));

    std::string source = ProgPow::getKern(period, ProgPow::KERNEL_CL);
    source += oggpow_cl;
    const std::string options = m_options +
        " -DGROUP_SIZE=" + std::to_string(m_workGroupSize) +
        " -DLIGHT_WORDS=" + std::to_string(lightWords) +
        " -DPROGPOW_DAG_BYTES=" + std::to_string(KPCache::dag_size(epoch)) +
        " -DPROGPOW_DAG_ELEMENTS=" + std::to_string(dagItems / 2) +
        " -DMAX_OUTPUTS=" + std::to_string(kMaxOutputs);

    const char *text = source.c_str();
    cl_int ret = CL_SUCCESS;
    m_oggProgram = OclLib::createProgramWithSource(m_ctx, 1, &text, nullptr, &ret);
    if (ret != CL_SUCCESS) {
        throw std::runtime_error(OclError::toString(ret));
    }

    const cl_device_id device = data().device.id();
    ret = OclLib::buildProgram(m_oggProgram, 1, &device, options.c_str());
    if (ret != CL_SUCCESS) {
        LOG_ERR("%s OggPoW OpenCL build failed: %s", ocl_tag(), OclLib::getProgramBuildLog(m_oggProgram, device).data());
        throw std::runtime_error(OclError::toString(ret));
    }

    m_searchKernel = OclLib::createKernel(m_oggProgram, "ethash_search", &ret);
    if (ret != CL_SUCCESS) {
        throw std::runtime_error(OclError::toString(ret));
    }
    m_dagKernel = OclLib::createKernel(m_oggProgram, "ethash_calculate_dag_item", &ret);
    if (ret != CL_SUCCESS) {
        throw std::runtime_error(OclError::toString(ret));
    }

    m_period = period;
    LOG_INFO("%s OggPoW OpenCL period %" PRIu64 " compiled", Tags::opencl(), period);
}

void OclOggPowRunner::set(const Job &job, uint8_t *blob)
{
    const uint32_t epoch = static_cast<uint32_t>(job.height() / kEpochLength);
    const uint64_t period = job.height() / 10;
    const bool epochChanged = epoch != m_epoch;

    {
        std::lock_guard<std::mutex> lock(KPCache::s_cacheMutex);
        if (!KPCache::s_cache.init(epoch, 256)) {
            throw std::runtime_error("invalid OggPoW epoch");
        }

        const size_t lightSize = KPCache::s_cache.size();
        if (lightSize > m_lightCacheCapacity) {
            OclLib::release(m_lightCache);
            m_lightCacheCapacity = VirtualMemory::align(lightSize);
            m_lightCache = OclLib::createBuffer(m_ctx, CL_MEM_READ_ONLY, m_lightCacheCapacity);
        }
        if (epochChanged) {
            m_epoch = epoch;
            m_lightCacheSize = lightSize;
            enqueueWriteBuffer(m_lightCache, CL_TRUE, 0, m_lightCacheSize, KPCache::s_cache.data());
        }
    }

    const size_t dagSize = KPCache::dag_size(epoch);
    if (dagSize > m_dagCapacity) {
        OclLib::release(m_dag);
        m_dagCapacity = VirtualMemory::align(dagSize, 16 * 1024 * 1024);
        m_dag = OclLib::createBuffer(m_ctx, CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS, m_dagCapacity);
    }

    if (period != m_period) {
        compileKernel(period);
    }

    if (epochChanged || m_dagKernel == nullptr) {
        const uint32_t dagItems = static_cast<uint32_t>(dagSize / sizeof(node));
        setArg(m_dagKernel, 1, sizeof(cl_mem), &m_lightCache);
        setArg(m_dagKernel, 2, sizeof(cl_mem), &m_dag);
        const uint32_t isolate = 1;
        setArg(m_dagKernel, 3, sizeof(isolate), &isolate);
        const size_t global = static_cast<size_t>(dagItems) * 2;
        const uint32_t start = 0;
        setArg(m_dagKernel, 0, sizeof(start), &start);
        // Let the AMD runtime choose the DAG kernel's local size.  The
        // generated search kernel has a required workgroup size, while the
        // DAG kernel intentionally does not; forcing the search size here
        // causes CL_INVALID_WORK_GROUP_SIZE on recent RDNA drivers.
        const cl_int ret = OclLib::enqueueNDRangeKernel(m_queue, m_dagKernel, 1, nullptr, &global, nullptr, 0, nullptr, nullptr);
        if (ret != CL_SUCCESS) {
            throw std::runtime_error(std::string("failed to enqueue OggPoW DAG: ") + OclError::toString(ret));
        }
        const cl_int finishRet = OclLib::finish(m_queue);
        if (finishRet != CL_SUCCESS) {
            throw std::runtime_error(std::string("failed to generate OggPoW DAG: ") + OclError::toString(finishRet));
        }
    }

    m_blob = blob;
    m_target = job.target();
    if (!setSearchArgs(0, m_target)) {
        throw std::runtime_error("failed to set OggPoW OpenCL arguments");
    }
}

bool OclOggPowRunner::setSearchArgs(uint64_t startNonce, uint64_t target)
{
    const uint32_t hackFalse = 0;
    return OclLib::setKernelArg(m_searchKernel, 0, sizeof(cl_mem), &m_output) == CL_SUCCESS &&
           OclLib::setKernelArg(m_searchKernel, 1, sizeof(cl_mem), &m_input) == CL_SUCCESS &&
           OclLib::setKernelArg(m_searchKernel, 2, sizeof(cl_mem), &m_dag) == CL_SUCCESS &&
           OclLib::setKernelArg(m_searchKernel, 3, sizeof(startNonce), &startNonce) == CL_SUCCESS &&
           OclLib::setKernelArg(m_searchKernel, 4, sizeof(target), &target) == CL_SUCCESS &&
           OclLib::setKernelArg(m_searchKernel, 5, sizeof(hackFalse), &hackFalse) == CL_SUCCESS;
}

void OclOggPowRunner::run(uint32_t nonce, uint32_t, uint32_t *hashOutput)
{
    // Ogg's OpenCL kernel receives the 32-byte header hash separately from a
    // 64-bit nonce.  The current stratum ABI supplies a 32-bit search window;
    // the upper half remains zero, which is valid for pool shares.
    enqueueWriteBuffer(m_input, CL_FALSE, 0, 32, m_blob);
    const uint32_t zero[kOutputWords] = {};
    enqueueWriteBuffer(m_output, CL_FALSE, 0, sizeof(zero), zero);
    const uint64_t startNonce = nonce;
    if (!setSearchArgs(startNonce, m_target)) {
        throw std::runtime_error("failed to update OggPoW nonce");
    }

    const size_t global = m_intensity - (m_intensity % m_workGroupSize);
    const size_t local = m_workGroupSize;
    const cl_int ret = OclLib::enqueueNDRangeKernel(m_queue, m_searchKernel, 1, nullptr, &global, &local, 0, nullptr, nullptr);
    if (ret != CL_SUCCESS) {
        throw std::runtime_error(OclError::toString(ret));
    }

    uint32_t output[kOutputWords] = {};
    enqueueReadBuffer(m_output, CL_TRUE, 0, sizeof(output), output);
    const uint32_t count = std::min(output[kResultWords * kMaxOutputs], kMaxOutputs);
    hashOutput[0xFF] = count;
    for (uint32_t i = 0; i < count; ++i) {
        hashOutput[i] = nonce + output[i * kResultWords];
        for (uint32_t word = 0; word < 8; ++word) {
            hashOutput[16 + i * 8 + word] = output[i * kResultWords + 1 + word];
        }
    }
    m_skippedHashes = 0;
}

void OclOggPowRunner::jobEarlyNotification(const Job&)
{
    const uint32_t one = 1;
    // SearchResults::abort is the cancellation flag used by ethash_search.
    OclLib::enqueueWriteBuffer(m_queue, m_output, CL_TRUE, 242 * sizeof(uint32_t), sizeof(one), &one, 0, nullptr, nullptr);
}

} // namespace xmrig
