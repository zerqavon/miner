#include "crypto/nexapow/NexaPow.h"

#include <openssl/ec.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/obj_mac.h>
#include <openssl/sha.h>

#include <array>
#include <cstring>

namespace xmrig {
namespace nexapow {

namespace {

using Bytes = std::array<uint8_t, 32>;

static Bytes sha256(const uint8_t *data, size_t size)
{
    Bytes out{};
    SHA256(data, size, out.data());
    return out;
}

static Bytes hmac256(const uint8_t *key, size_t keySize, const uint8_t *data, size_t dataSize)
{
    Bytes out{};
    unsigned int size = 0;
    HMAC(EVP_sha256(), key, static_cast<int>(keySize), data, dataSize, out.data(), &size);
    return out;
}

static Bytes rfc6979(const Bytes &key, const Bytes &msg)
{
    // libsecp256k1's RFC6979 nonce function feeds key || message ||
    // "Schnorr+SHA256  " into the HMAC-DRBG.
    static constexpr uint8_t algo[] = "Schnorr+SHA256  ";
    std::array<uint8_t, 80> seed{};
    memcpy(seed.data(), key.data(), 32);
    memcpy(seed.data() + 32, msg.data(), 32);
    memcpy(seed.data() + 64, algo, 16);

    Bytes K{};
    K.fill(0);
    Bytes V{};
    V.fill(1);

    std::array<uint8_t, 113> input{};
    memcpy(input.data(), V.data(), 32);
    input[32] = 0;
    memcpy(input.data() + 33, seed.data(), seed.size());
    K = hmac256(K.data(), K.size(), input.data(), 113);
    V = hmac256(K.data(), K.size(), V.data(), V.size());

    memcpy(input.data(), V.data(), 32);
    input[32] = 1;
    memcpy(input.data() + 33, seed.data(), seed.size());
    K = hmac256(K.data(), K.size(), input.data(), 113);
    V = hmac256(K.data(), K.size(), V.data(), V.size());

    return hmac256(K.data(), K.size(), V.data(), V.size());
}

static bool fixed32(const BIGNUM *value, uint8_t out[32])
{
    return BN_bn2binpad(value, out, 32) == 32;
}

}

bool hash(const uint8_t *headerCommitment, const uint8_t *nonce16, uint8_t out[32])
{
    // Nexa's RPC/Stratum header commitment is displayed in digest order,
    // while uint256 serialization stores it little-endian.  GetMiningHash
    // serializes uint256 followed by a CompactSize-prefixed byte vector.
    // Echelon's solution nonce is a 16-byte vector, therefore the exact
    // preimage is: reverse(commitment) || 0x10 || nonce[16].
    uint8_t candidate[49]{};
    for (size_t i = 0; i < 32; ++i) candidate[i] = headerCommitment[31 - i];
    candidate[32] = 16;
    memcpy(candidate + 33, nonce16, 16);

    const Bytes miningHash = sha256(sha256(candidate, sizeof(candidate)).data(), 32);
    return finalize(miningHash.data(), out);
}

bool hashLegacy(const uint8_t *headerCommitment, const uint8_t *nonce8, uint8_t out[32])
{
    uint8_t candidate[41]{};
    for (size_t i = 0; i < 32; ++i) candidate[i] = headerCommitment[31 - i];
    candidate[32] = 8;
    memcpy(candidate + 33, nonce8, 8);
    const Bytes miningHash = sha256(sha256(candidate, sizeof(candidate)).data(), 32);
    return finalize(miningHash.data(), out);
}

bool finalize(const uint8_t miningHashData[32], uint8_t out[32])
{
    const Bytes miningHash = *reinterpret_cast<const Bytes *>(miningHashData);
    const Bytes h1 = sha256(miningHash.data(), miningHash.size());

    bool valid = false;
    // Finalization is called for every candidate returned by OpenCL. Keep
    // the secp256k1 objects alive for this worker thread instead of creating
    // and destroying them for every nonce.
    static thread_local EC_GROUP *group = EC_GROUP_new_by_curve_name(NID_secp256k1);
    static thread_local BN_CTX *ctx = BN_CTX_new();
    static thread_local BIGNUM *priv = BN_new();
    static thread_local BIGNUM *order = BN_new();
    static thread_local EC_POINT *pub = group ? EC_POINT_new(group) : nullptr;
    static thread_local EC_POINT *R = group ? EC_POINT_new(group) : nullptr;
    static thread_local BIGNUM *rx = BN_new();
    static thread_local BIGNUM *ry = BN_new();
    static thread_local BIGNUM *k = BN_new();
    static thread_local BIGNUM *e = BN_new();
    static thread_local BIGNUM *s = BN_new();
    static thread_local BIGNUM *exp = BN_new();
    static thread_local BIGNUM *jacobi = BN_new();
    Bytes nonceBytes{};
    Bytes challengeHash{};
    uint8_t compressed[33]{};
    uint8_t challenge[97]{};
    uint8_t signature[64]{};

    if (!group || !ctx || !priv || !order || !pub || !R || !rx || !ry || !k || !e || !s) {
        goto cleanup;
    }

    BN_bin2bn(miningHash.data(), 32, priv);
    EC_GROUP_get_order(group, order, ctx);
    if (BN_is_zero(priv) || BN_cmp(priv, order) >= 0) {
        goto cleanup;
    }

    if (EC_POINT_mul(group, pub, priv, nullptr, nullptr, ctx) != 1) {
        goto cleanup;
    }

    nonceBytes = rfc6979(*reinterpret_cast<const Bytes *>(miningHash.data()), h1);
    BN_bin2bn(nonceBytes.data(), 32, k);
    if (BN_mod(k, k, order, ctx) != 1 || BN_is_zero(k)) {
        goto cleanup;
    }

    if (EC_POINT_mul(group, R, k, nullptr, nullptr, ctx) != 1 ||
        EC_POINT_get_affine_coordinates(group, R, rx, ry, ctx) != 1) {
        goto cleanup;
    }

    // Nexa's Schnorr variant requires the Jacobi symbol of R.y to be +1.
    BN_one(exp);
    BN_sub(exp, EC_GROUP_get0_field(group), BN_value_one());
    BN_rshift1(exp, exp);
    if (BN_mod_exp(jacobi, ry, exp, EC_GROUP_get0_field(group), ctx) != 1) goto cleanup;
    if (!BN_is_one(jacobi)) {
        BN_sub(k, order, k);
        EC_POINT_invert(group, R, ctx);
        EC_POINT_get_affine_coordinates(group, R, rx, ry, ctx);
    }
    compressed[0] = static_cast<uint8_t>(BN_is_odd(ry) ? 0x03 : 0x02);
    if (!fixed32(rx, compressed + 1)) goto cleanup;

    fixed32(rx, challenge);
    memcpy(challenge + 32, compressed, 33);
    memcpy(challenge + 65, h1.data(), 32);
    challengeHash = sha256(challenge, sizeof(challenge));
    BN_bin2bn(challengeHash.data(), 32, e);
    BN_mod(e, e, order, ctx);
    BN_mod_mul(s, e, priv, order, ctx);
    BN_mod_add(s, s, k, order, ctx);

    if (!fixed32(rx, signature) || !fixed32(s, signature + 32)) goto cleanup;
    memcpy(out, sha256(signature, sizeof(signature)).data(), 32);
    valid = true;

cleanup:
    return valid;
}

}
}
