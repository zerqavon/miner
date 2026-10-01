#ifndef LIQUIDMINER_VULKAN_KAWPOW_RUNNER_H
#define LIQUIDMINER_VULKAN_KAWPOW_RUNNER_H

#include "backend/opencl/interfaces/IOclRunner.h"
#include "backend/vulkan/VulkanKawPowApi.h"

namespace xmrig {

class OclLaunchData;

class VulkanKawPowRunner final : public IOclRunner
{
public:
    VulkanKawPowRunner(size_t index, const OclLaunchData &data);
    ~VulkanKawPowRunner() override;

    bool available() const { return m_ctx != nullptr; }

protected:
    cl_context ctx() const override;
    const Algorithm &algorithm() const override;
    const char *buildOptions() const override { return ""; }
    const char *deviceKey() const override { return "liquidminer-vulkan-kawpow"; }
    const char *source() const override { return ""; }
    const OclLaunchData &data() const override { return m_data; }
    size_t intensity() const override;
    size_t threadId() const override { return m_index; }
    uint32_t roundSize() const override;
    uint32_t processedHashes() const override { return m_processed; }
    uint32_t deviceIndex() const override;
    void build() override {}
    void init() override;
    void run(uint32_t nonce, uint32_t nonce_offset, uint32_t *hashOutput) override;
    void set(const Job &job, uint8_t *blob) override;
    void jobEarlyNotification(const Job &) override {}

protected:
    size_t bufferSize() const override { return 0; }

private:
    size_t m_index;
    const OclLaunchData &m_data;
    VulkanMiner *m_ctx = nullptr;
    uint8_t *m_blob = nullptr;
    uint32_t m_intensity = 0;
    uint32_t m_processed = 0;
    uint64_t m_nonceBase = 0;
    uint64_t m_target = 0;
    uint64_t m_height = 0;
};

} // namespace xmrig

#endif
