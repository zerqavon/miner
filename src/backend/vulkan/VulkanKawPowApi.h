/*
 * LiquidMiner Vulkan KawPow bridge.
 *
 * The implementation is provided by the GPL Vulkan host in
 * thirdparty-bench/rdna3-kawpow-miner.  Keep this header C-compatible so the
 * GPU worker does not depend on Rust/Vulkan types or their lifetimes.
 */
#ifndef LIQUIDMINER_VULKAN_KAWPOW_API_H
#define LIQUIDMINER_VULKAN_KAWPOW_API_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct VulkanMiner VulkanMiner;

VulkanMiner *lm_vk_kawpow_create(uint32_t device_index,
                                  const char *cache_dir,
                                  uint32_t local_size);

void lm_vk_kawpow_destroy(VulkanMiner *ctx);

int32_t lm_vk_kawpow_set_job(VulkanMiner *ctx,
                             uint64_t height,
                             const uint8_t *header,
                             uint64_t target,
                             uint64_t start_nonce);

int32_t lm_vk_kawpow_search(VulkanMiner *ctx,
                            const uint8_t *header,
                            uint64_t target,
                            uint64_t start_nonce,
                            uint64_t num_nonces,
                            uint64_t *out_nonce,
                            uint8_t *out_mix,
                            uint32_t max_outputs);

const char *lm_vk_kawpow_last_error(void);

#ifdef __cplusplus
}
#endif

#endif
