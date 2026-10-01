#ifndef XMRIG_OCLOGGPOWRUNNER_H
#define XMRIG_OCLOGGPOWRUNNER_H

#include "backend/opencl/runners/OclBaseRunner.h"
#include "crypto/kawpow/KPCache.h"

namespace xmrig {

class OclOggPowRunner final : public OclBaseRunner
{
public:
    XMRIG_DISABLE_COPY_MOVE_DEFAULT(OclOggPowRunner)

    OclOggPowRunner(size_t index, const OclLaunchData &data);
    ~OclOggPowRunner() override;

protected:
    void run(uint32_t nonce, uint32_t nonce_offset, uint32_t *hashOutput) override;
    void set(const Job &job, uint8_t *blob) override;
    void build() override;
    void init() override;
    void jobEarlyNotification(const Job &job) override;
    uint32_t roundSize() const override { return m_tunedIntensity; }
    uint32_t processedHashes() const override { return m_tunedIntensity - m_skippedHashes; }

private:
    void compileKernel(uint64_t period);
    bool setSearchArgs(uint64_t startNonce, uint64_t target);

    cl_program m_oggProgram = nullptr;
    cl_kernel m_searchKernel = nullptr;
    cl_kernel m_dagKernel = nullptr;
    cl_mem m_lightCache = nullptr;
    cl_mem m_dag = nullptr;
    cl_mem m_stop = nullptr;

    size_t m_lightCacheSize = 0;
    size_t m_lightCacheCapacity = 0;
    size_t m_dagCapacity = 0;
    uint32_t m_epoch = 0xFFFFFFFFUL;
    uint64_t m_period = UINT64_MAX;
    uint64_t m_target = 0;
    uint32_t m_workGroupSize = 256;
    uint32_t m_dagWorkGroupSize = 64;
    uint32_t m_tunedIntensity = 0;
    uint32_t m_skippedHashes = 0;
    uint8_t *m_blob = nullptr;
};

} // namespace xmrig

#endif
