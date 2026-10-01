#ifndef XMRIG_NEXAPOW_H
#define XMRIG_NEXAPOW_H

#include <cstdint>

namespace xmrig {
namespace nexapow {

bool hash(const uint8_t *headerCommitment, const uint8_t *nonce16, uint8_t out[32]);
bool hashLegacy(const uint8_t *headerCommitment, const uint8_t *nonce8, uint8_t out[32]);
bool finalize(const uint8_t miningHash[32], uint8_t out[32]);

}
}

#endif
