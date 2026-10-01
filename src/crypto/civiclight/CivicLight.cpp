/* XMRig CivicLight PoW */

#include "crypto/civiclight/CivicLight.h"
#include "crypto/civiclight/yespower/yespower.h"
#include "crypto/civiclight/yespower/yespower_sha256.h"


#include <cstring>


namespace xmrig {
namespace civiclight {


static constexpr uint32_t kV2ActivationTime = 1784797200U;


static inline uint32_t read32le(const uint8_t *data)
{
    return static_cast<uint32_t>(data[0])
        | (static_cast<uint32_t>(data[1]) << 8)
        | (static_cast<uint32_t>(data[2]) << 16)
        | (static_cast<uint32_t>(data[3]) << 24);
}


static inline void sha256d(void *output, const void *input, size_t size)
{
    uint8_t tmp[32];

    YP_SHA256_Buf(input, size, tmp);
    YP_SHA256_Buf(tmp, sizeof(tmp), static_cast<uint8_t *>(output));
}


static inline void coreV1(void *output, const void *input, size_t size)
{
    uint8_t hash1[32];

    YP_SHA256_Buf(input, size, hash1);

    for (uint8_t &v : hash1) {
        v ^= 0x5A;
    }

    YP_SHA256_Buf(hash1, sizeof(hash1), static_cast<uint8_t *>(output));
}


static inline void coreV2(void *output, const void *input, size_t size)
{
    uint8_t hash1[32];
    uint8_t xorBuf[32];

    YP_SHA256_Buf(input, size, hash1);

    civic_yespower_params_t params;
    params.version = YESPOWER_1_0;
    params.N       = 2048;
    params.r       = 8;
    params.pers    = nullptr;
    params.perslen = 0;

    civic_yespower_binary_t ypOut;
    if (civic_yespower_tls(hash1, sizeof(hash1), &params, &ypOut) != 0) {
        std::memset(output, 0, 32);
        return;
    }

    for (size_t i = 0; i < sizeof(xorBuf); ++i) {
        xorBuf[i] = ypOut.uc[i] ^ hash1[i];
    }

    YP_SHA256_Buf(xorBuf, sizeof(xorBuf), static_cast<uint8_t *>(output));
}


void hash(const void *input, size_t size, void *output)
{
    if (size < 80) {
        std::memset(output, 0, 32);
        return;
    }

    uint8_t intermediate[32];
    sha256d(intermediate, input, 80);

    const auto *header = static_cast<const uint8_t *>(input);
    const uint32_t ntime = read32le(header + 68);

    if (ntime >= kV2ActivationTime) {
        coreV2(output, intermediate, sizeof(intermediate));
    }
    else {
        coreV1(output, intermediate, sizeof(intermediate));
    }
}


} // namespace civiclight
} // namespace xmrig
