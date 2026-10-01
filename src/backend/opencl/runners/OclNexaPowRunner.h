#ifndef XMRIG_OCLNEXAPOWRUNNER_H
#define XMRIG_OCLNEXAPOWRUNNER_H

#include "backend/opencl/runners/OclBaseRunner.h"

#if defined(_WIN32)
#   include <hip/hip_runtime_api.h>
#   include <hip/hiprtc.h>
#endif

namespace xmrig {
class OclNexaPowRunner final : public OclBaseRunner {
public:
    OclNexaPowRunner(size_t index, const OclLaunchData &data);
    ~OclNexaPowRunner() override;
protected:
    void run(uint32_t nonce, uint32_t nonce_offset, uint32_t *hashOutput) override;
    void set(const Job &job, uint8_t *blob) override;
    void build() override;
    void init() override;
    uint32_t roundSize() const override { return m_batch; }
    uint32_t processedHashes() const override { return m_batch; }
private:
    cl_kernel m_kernel = nullptr;
    cl_mem m_hashes = nullptr;
    uint32_t m_batch = 256;
    uint32_t m_workGroupSize = 64;
    uint8_t m_blob[40]{};
    uint64_t m_target = 0;
#if defined(_WIN32)
    hiprtcProgram m_hipRtc = nullptr;
    hipModule_t m_hipModule = nullptr;
    hipFunction_t m_hipFunction = nullptr;
    hipDeviceptr_t m_hipInput = 0;
    hipDeviceptr_t m_hipHashes = 0;
    bool m_hipReady = false;
#endif
};
}

#endif
