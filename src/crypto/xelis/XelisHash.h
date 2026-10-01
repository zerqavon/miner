#ifndef LIQUIDMINER_XELIS_HASH_H
#define LIQUIDMINER_XELIS_HASH_H

#include <stdint.h>

#define LIQUIDMINER_XELIS_INPUT_SIZE 112
#define LIQUIDMINER_XELIS_SCRATCH_WORDS (531 * 128)

#ifdef __cplusplus
extern "C" {
#endif
void xelis_hash_v3(uint8_t in[LIQUIDMINER_XELIS_INPUT_SIZE], uint8_t hash[32], uint64_t scratch[LIQUIDMINER_XELIS_SCRATCH_WORDS]);
#ifdef __cplusplus
}
#endif

#endif
