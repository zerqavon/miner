#include "backend/vulkan/VulkanKawPowRunner.h"

#include <cstring>
#include <stdexcept>

#include "backend/opencl/OclLaunchData.h"
#include "base/io/log/Log.h"
#include "base/io/log/Tags.h"
#include "base/net/stratum/Job.h"

namespace xmrig {

VulkanKawPowRunner::VulkanKawPowRunner(size_t index, const OclLaunchData &data) :
    m_index(index),
    m_data(data),
    m_intensity(static_cast<uint32_t>(data.thread.intensity()))
{
    const uint32_t local_size = static_cast<uint32_t>(data.thread.worksize());
    const char *cache = "cache/kawpow-vulkan";
    m_ctx = lm_vk_kawpow_create(static_cast<uint32_t>(data.device.index()), cache, local_size);
    if (!m_ctx) {
        const char *error = lm_vk_kawpow_last_error();
        LOG_WARN("%s Vulkan KawPow unavailable: %s", Tags::miner(), error ? error : "unknown error");
    }
}

VulkanKawPowRunner::~VulkanKawPowRunner()
{
    lm_vk_kawpow_destroy(m_ctx);
}

cl_context VulkanKawPowRunner::ctx() const
{
    return m_data.ctx;
}

const Algorithm &VulkanKawPowRunner::algorithm() const
{
    return m_data.algorithm;
}

size_t VulkanKawPowRunner::intensity() const
{
    return m_intensity;
}

uint32_t VulkanKawPowRunner::roundSize() const
{
    return m_intensity;
}

uint32_t VulkanKawPowRunner::deviceIndex() const
{
    return static_cast<uint32_t>(m_data.device.index());
}

void VulkanKawPowRunner::init()
{
    if (!m_ctx) {
        throw std::runtime_error("Vulkan KawPow context is not available");
    }
}

void VulkanKawPowRunner::set(const Job &job, uint8_t *blob)
{
    if (!m_ctx || !blob) {
        throw std::runtime_error("Vulkan KawPow context is not available");
    }

    m_blob = blob;
    m_height = job.height();
    m_target = job.target();
    std::memcpy(&m_nonceBase, blob + 32, sizeof(m_nonceBase));

    if (lm_vk_kawpow_set_job(m_ctx, m_height, blob, m_target, m_nonceBase) != 0) {
        const char *error = lm_vk_kawpow_last_error();
        throw std::runtime_error(error ? error : "failed to prepare Vulkan KawPow job");
    }
}

void VulkanKawPowRunner::run(uint32_t nonce, uint32_t, uint32_t *hashOutput)
{
    if (!m_ctx || !m_blob || !hashOutput) {
        throw std::runtime_error("Vulkan KawPow runner is not ready");
    }

    uint64_t found[16] = {};
    uint8_t mixes[16][32] = {};
    const uint64_t start = m_nonceBase + static_cast<uint64_t>(nonce);
    const int32_t count = lm_vk_kawpow_search(m_ctx, m_blob, m_target, start, m_intensity,
                                               found, &mixes[0][0], 16);
    if (count < 0) {
        const char *error = lm_vk_kawpow_last_error();
        throw std::runtime_error(error ? error : "Vulkan KawPow search failed");
    }

    hashOutput[0xFF] = static_cast<uint32_t>(count);
    for (int32_t i = 0; i < count; ++i) {
        // JobResults writes this low word into the existing 64-bit KawPow
        // nonce, preserving the pool-provided high/extranonce word.
        hashOutput[i] = static_cast<uint32_t>(found[i]);
    }
    m_processed = m_intensity;
}

} // namespace xmrig
