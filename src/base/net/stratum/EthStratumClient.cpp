/* XMRig
 * Copyright (c) 2018-2021 SChernykh   <https://github.com/SChernykh>
 * Copyright (c) 2016-2021 XMRig       <https://github.com/xmrig>, <support@xmrig.com>
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <cinttypes>
#include <iomanip>
#include <sstream>
#include <stdexcept>


#include "base/net/stratum/EthStratumClient.h"
#include "3rdparty/libethash/endian.h"
#include "3rdparty/rapidjson/document.h"
#include "3rdparty/rapidjson/error/en.h"
#include "3rdparty/rapidjson/stringbuffer.h"
#include "3rdparty/rapidjson/writer.h"
#include "base/io/json/Json.h"
#include "base/io/json/JsonRequest.h"
#include "base/io/log/Log.h"
#include "base/kernel/interfaces/IClientListener.h"
#include "net/JobResult.h"

#ifdef XMRIG_ALGO_GHOSTRIDER
#include <cmath>

extern "C" {
#include "crypto/ghostrider/sph_sha2.h"
}

#include "base/tools/Cvt.h"
#endif



xmrig::EthStratumClient::EthStratumClient(int id, const char *agent, IClientListener *listener) :
    Client(id, agent, listener)
{
}


int64_t xmrig::EthStratumClient::submit(const JobResult& result)
{
#   ifndef XMRIG_PROXY_PROJECT
    if ((m_state != ConnectedState) || !m_authorized) {
        return -1;
    }
#   endif

    const uint64_t requiredDiff = (m_oggProxyMode && result.diff == 0) ? m_job.diff() : result.diff;
    if (requiredDiff == 0) {
        LOG_ERR("%s " RED("result.diff is 0"), tag());
        close();

        return -1;
    }

    using namespace rapidjson;

    Document doc(kObjectType);
    auto& allocator = doc.GetAllocator();

    Value params(kArrayType);

    // Oggchain's official pool exposes the eth-proxy (stratum1) API.  It
    // does not accept mining.submit and does not use a subscription response;
    // solutions are submitted as eth_submitWork(header nonce, header, mix).
    if (m_oggProxyMode) {
        std::stringstream s;
        s << "0x" << std::hex << std::setw(16) << std::setfill('0') << result.nonce;
        params.PushBack(Value(s.str().c_str(), allocator), allocator);

        s.str(std::string());
        s << "0x";
        for (size_t i = 0; i < 32; ++i) {
            s << std::hex << std::setw(2) << std::setfill('0') << static_cast<unsigned>(result.headerHash()[i]);
        }
        params.PushBack(Value(s.str().c_str(), allocator), allocator);

        s.str(std::string());
        s << "0x";
        for (size_t i = 0; i < 32; ++i) {
            s << std::hex << std::setw(2) << std::setfill('0') << static_cast<unsigned>(result.mixHash()[i]);
        }
        params.PushBack(Value(s.str().c_str(), allocator), allocator);

        JsonRequest::create(doc, m_sequence, "eth_submitWork", params);

        const uint64_t actual_diff = ethash_swap_u64(*((uint64_t*)result.result()));
        const uint64_t share_diff = actual_diff ? (uint64_t(-1) / actual_diff) : 0;
#       ifdef XMRIG_PROXY_PROJECT
        m_results[m_sequence] = SubmitResult(m_sequence, requiredDiff, share_diff, result.id, 0, result.switchOnAccept);
#       else
        m_results[m_sequence] = SubmitResult(m_sequence, requiredDiff, share_diff, 0, result.backend, result.switchOnAccept);
#       endif

        return send(doc);
    }

    params.PushBack(m_user.toJSON(), allocator);
    params.PushBack(result.jobId.toJSON(), allocator);

#   ifdef XMRIG_ALGO_GHOSTRIDER
    if ((m_pool.algorithm().id() == Algorithm::GHOSTRIDER_RTM) || (m_pool.algorithm().id() == Algorithm::CIVICLIGHT) || isVextaStratum()) {
        params.PushBack(Value("00000000000000000000000000000000", static_cast<uint32_t>(m_extraNonce2Size * 2)), allocator);
        params.PushBack(Value(m_ntime.data(), allocator), allocator);

        std::stringstream s;
        s << std::hex << std::setw(8) << std::setfill('0') << result.nonce;
        params.PushBack(Value(s.str().c_str(), allocator), allocator);
    }
    else
#   endif
    {
        std::stringstream s;
        s << "0x" << std::hex << std::setw(16) << std::setfill('0') << result.nonce;
        params.PushBack(Value(s.str().c_str(), allocator), allocator);

        s.str(std::string());
        s << "0x";
        for (size_t i = 0; i < 32; ++i) {
            const uint32_t k = result.headerHash()[i];
            s << std::hex << std::setw(2) << std::setfill('0') << k;
        }
        params.PushBack(Value(s.str().c_str(), allocator), allocator);

        s.str(std::string());
        s << "0x";
        for (size_t i = 0; i < 32; ++i) {
            const uint32_t k = result.mixHash()[i];
            s << std::hex << std::setw(2) << std::setfill('0') << k;
        }
        params.PushBack(Value(s.str().c_str(), allocator), allocator);
    }

    JsonRequest::create(doc, m_sequence, "mining.submit", params);

    uint64_t actual_diff;

#   ifdef XMRIG_ALGO_GHOSTRIDER
    if ((result.algorithm == Algorithm::GHOSTRIDER_RTM) || (result.algorithm == Algorithm::CIVICLIGHT) || isVextaStratum()) {
        actual_diff = reinterpret_cast<const uint64_t*>(result.result())[3];
    }
    else
#   endif
    {
        actual_diff = ethash_swap_u64(*((uint64_t*)result.result()));
    }

    actual_diff = actual_diff ? (uint64_t(-1) / actual_diff) : 0;

#   ifdef XMRIG_PROXY_PROJECT
    m_results[m_sequence] = SubmitResult(m_sequence, result.diff, actual_diff, result.id, 0, result.switchOnAccept);
#   else
    m_results[m_sequence] = SubmitResult(m_sequence, result.diff, actual_diff, 0, result.backend, result.switchOnAccept);
#   endif

    return send(doc);
}


void xmrig::EthStratumClient::login()
{
    m_results.clear();

    m_oggProxyMode = m_pool.algorithm().family() == Algorithm::OGGPOW_FAMILY;
    m_nextOggWork = 0;

    if (m_oggProxyMode) {
        oggLogin();
        return;
    }

    subscribe();
    authorize();
}


void xmrig::EthStratumClient::onClose()
{
    m_authorized = false;
    m_oggProxyMode = false;
    m_nextOggWork = 0;
    Client::onClose();
}


void xmrig::EthStratumClient::tick(uint64_t now)
{
    Client::tick(now);

    if (m_oggProxyMode && m_authorized && now >= m_nextOggWork) {
        requestOggWork();
    }
}


bool xmrig::EthStratumClient::handleResponse(int64_t id, const rapidjson::Value &result, const rapidjson::Value &error)
{
    auto it = m_callbacks.find(id);
    if (it != m_callbacks.end()) {
        const uint64_t elapsed = Chrono::steadyMSecs() - it->second.ts;

        if (error.IsArray() || error.IsObject() || error.IsString()) {
            it->second.callback(error, false, elapsed);
        }
        else {
            it->second.callback(result, true, elapsed);
        }

        m_callbacks.erase(it);

        return true;
    }

    return handleSubmitResponse(id, errorMessage(error));
}


void xmrig::EthStratumClient::parseNotification(const char *method, const rapidjson::Value &params, const rapidjson::Value &)
{
    if (strcmp(method, "mining.set_target") == 0) {
        return;
    }

    if (strcmp(method, "mining.set_extranonce") == 0) {
        if (!params.IsArray()) {
            LOG_ERR("%s " RED("invalid mining.set_extranonce notification: params is not an array"), tag());
            return;
        }

        auto arr = params.GetArray();

        if (arr.Empty()) {
            LOG_ERR("%s " RED("invalid mining.set_extranonce notification: params array is empty"), tag());
            return;
        }

        setExtraNonce(arr[0]);
    }

#   ifdef XMRIG_ALGO_GHOSTRIDER
    if (strcmp(method, "mining.set_difficulty") == 0) {
        if (!params.IsArray()) {
            LOG_ERR("%s " RED("invalid mining.set_difficulty notification: params is not an array"), tag());
            return;
        }

        if ((m_pool.algorithm().id() != Algorithm::GHOSTRIDER_RTM) && (m_pool.algorithm().id() != Algorithm::CIVICLIGHT) && !isVextaStratum()) {
            return;
        }

        auto arr = params.GetArray();

        if (arr.Empty()) {
            LOG_ERR("%s " RED("invalid mining.set_difficulty notification: params array is empty"), tag());
            return;
        }

        if (!arr[0].IsDouble() && !arr[0].IsUint64()) {
            LOG_ERR("%s " RED("invalid mining.set_difficulty notification: difficulty is not a number"), tag());
            return;
        }

        const double diff = arr[0].IsDouble() ? arr[0].GetDouble() : arr[0].GetUint64();

        if (isVextaStratum()) {
            // Vexta pools may deliberately use fractional difficulties for
            // testing and for low-hashrate miners.  Keep the original value;
            // rounding it to 1 makes a pool diff such as 5e-6 about 200,000
            // times harder than intended.
            m_nextVextaDifficulty = diff > 0.0 ? diff : 0.0;
            m_nextDifficulty = diff > 0.0 ? static_cast<uint64_t>(ceil(diff)) : 0;
        }
        else if (m_pool.algorithm().id() == Algorithm::CIVICLIGHT) {
            // CivicLight uses Bitcoin-style Stratum difficulty where diff 1
            // targets one share per 2^32 hashes. XMRig's CPU submit check uses
            // a 64-bit target, so convert pool difficulty to the equivalent
            // internal difficulty scale.
            m_nextDifficulty = std::max<uint64_t>(1, static_cast<uint64_t>(ceil(diff * 4295032833.000015)));
        }
        else {
            m_nextDifficulty = static_cast<uint64_t>(ceil(diff * 65536.0));
        }
    }
#   endif

    if (strcmp(method, "mining.notify") == 0) {
        if (!params.IsArray()) {
            LOG_ERR("%s " RED("invalid mining.notify notification: params is not an array"), tag());
            return;
        }

        auto arr = params.GetArray();

        auto algo = m_pool.algorithm();
        if (!algo.isValid()) {
            algo = m_pool.coin().algorithm();
        }

        const size_t min_arr_size = ((algo.id() == Algorithm::GHOSTRIDER_RTM) || (algo.id() == Algorithm::CIVICLIGHT)) ? 8 : (isVextaStratum() ? 10 : 6);

        if (arr.Size() < min_arr_size) {
            LOG_ERR("%s " RED("invalid mining.notify notification: params array has wrong size"), tag());
            return;
        }

        if (!arr[0].IsString()) {
            LOG_ERR("%s " RED("invalid mining.notify notification: invalid job id"), tag());
            return;
        }

        Job job;
        job.setId(arr[0].GetString());

        job.setAlgorithm(algo);
        job.setExtraNonce(m_extraNonce.second);

        std::stringstream s;

#       ifdef XMRIG_ALGO_GHOSTRIDER
        if ((algo.id() == Algorithm::GHOSTRIDER_RTM) || (algo.id() == Algorithm::CIVICLIGHT) || isVextaStratum()) {
            // Raptoreum uses Bitcoin's Stratum protocol
            // https://en.bitcoinwiki.org/wiki/Stratum_mining_protocol#mining.notify

            if (!arr[1].IsString() || !arr[2].IsString() || !arr[3].IsString() || !arr[4].IsArray() || !arr[5].IsString() || !arr[6].IsString() || !arr[7].IsString()) {
                LOG_ERR("%s " RED("invalid mining.notify notification: invalid param array"), tag());
                return;
            }

            // Version
            s << arr[5].GetString();

            // Previous block hash
            s << arr[1].GetString();

            // Merkle tree root
            std::string blob = arr[2].GetString();
            blob += m_extraNonce.second;
            blob.append(m_extraNonce2Size * 2, '0');
            blob += arr[3].GetString();

            uint8_t merkle_root[64];

            Buffer buf = Cvt::fromHex(blob.c_str(), blob.length());

            // Get height from coinbase
            {
                uint8_t* p = buf.data() + 32;
                uint8_t* m = p + 128;

                while ((p < m) && (*p != 0xff)) ++p;
                while ((p < m) && (*p == 0xff)) ++p;

                if ((p < m) && (*(p - 1) == 0xff) && (*(p - 2) == 0xff)) {
                    uint32_t height = *reinterpret_cast<uint16_t*>(p + 2);
                    switch (*(p + 1)) {
                    case 4:
                        height += *reinterpret_cast<uint16_t*>(p + 4) * 0x10000UL;
                        break;
                    case 3:
                        height += *(p + 4) * 0x10000UL;
                        break;
                    }
                    job.setHeight(height);
                }
                else {
                    job.setHeight(0);
                }
            }

            sha256d(merkle_root, buf.data(), static_cast<int>(buf.size()));

            auto merkle_branches = arr[4].GetArray();
            for (int i = 0, n = merkle_branches.Size(); i < n; ++i) {
                auto& b = merkle_branches[i];
                buf = b.IsString() ? Cvt::fromHex(b.GetString(), b.GetStringLength()) : Buffer();
                if (buf.size() != 32) {
                    LOG_ERR("%s " RED("invalid mining.notify notification: param 4 is invalid"), tag());
                    return;
                }
                memcpy(merkle_root + 32, buf.data(), 32);
                sha256d(merkle_root, merkle_root, 64);
            }

            s << Cvt::toHex(merkle_root, 32);

            // ntime
            m_ntime = arr[7].GetString();
            s << m_ntime;

            // nbits
            s << arr[6].GetString();

            blob = s.str();

            if (blob.size() != 76 * 2) {
                LOG_ERR("%s " RED("invalid mining.notify notification: invalid blob size"), tag());
                return;
            }

            // zeros up to 80 bytes
            blob.resize(80 * 2, '0');

            // Invert byte order (no idea why, but it's done in Bitcoin's Stratum)
            buf = Cvt::fromHex(blob.c_str(), blob.length());
            for (size_t i = 0; i < 80; i += sizeof(uint32_t)) {
                uint32_t& k = *reinterpret_cast<uint32_t*>(buf.data() + i);
                if ((i < 36) || (i >= 68)) {
                    k = ethash_swap_u32(k);
                }
            }
            blob = Cvt::toHex(buf.data(), buf.size());

            if (!job.setBlob(blob.c_str())) {
                LOG_ERR("%s " RED("invalid mining.notify notification: invalid block header"), tag());
                return;
            }

            if (isVextaStratum()) {
                // Vexta pools have used both orders in the wild:
                // [.., ntime, clean_jobs, randomx_seed] and
                // [.., ntime, randomx_seed, clean_jobs]. Accept either,
                // while validating the seed as a 32-byte hex value.
                bool validSeed = false;
                if (arr[9].IsString()) {
                    validSeed = job.setSeedHash(arr[9].GetString());
                }
                if (!validSeed && arr[8].IsString()) {
                    validSeed = job.setSeedHash(arr[8].GetString());
                }
                if (!validSeed) {
                    LOG_ERR("%s " RED("invalid mining.notify notification: Vexta RandomX seed is missing or invalid"), tag());
                    return;
                }
            }

            if (isVextaStratum()) {
                job.setDiff(m_nextVextaDifficulty);
            }
            else {
                job.setDiff(m_nextDifficulty);
            }
        }
        else
#       endif
        {
            // header hash (32 bytes)
            s << arr[1].GetString();

            // nonce template (8 bytes)
            for (uint64_t i = 0, k = m_extraNonce.first; i < sizeof(m_extraNonce.first); ++i, k >>= 8) {
                s << std::hex << std::setw(2) << std::setfill('0') << (k & 0xFF);
            }

            std::string blob = s.str();

            // zeros up to 76 bytes
            blob.resize(76 * 2, '0');
            job.setBlob(blob.c_str());

            std::string target_str = arr[3].GetString();
            target_str.resize(16, '0');
            const uint64_t target = strtoull(target_str.c_str(), nullptr, 16);
            job.setDiff(Job::toDiff(target));

            job.setHeight(arr[5].GetUint64());
        }

        bool ok = true;
        m_listener->onVerifyAlgorithm(this, algo, &ok);

        if (!ok) {
            if (!isQuiet()) {
                LOG_ERR("[%s] incompatible/disabled algorithm \"%s\" detected, reconnect", url(), algo.name());
            }
            close();
            return;
        }

        if (m_job != job) {
            m_job = std::move(job);

            // Workaround for nanopool.org, mining.notify received before mining.authorize response.
            if (!m_authorized) {
                m_authorized = true;
                m_listener->onLoginSuccess(this);
            }

            m_listener->onJobReceived(this, m_job, params);
        }
        else {
            if (!isQuiet()) {
                LOG_WARN("%s " YELLOW("duplicate job received, reconnect"), tag());
            }
            close();
        }
    }
}


void xmrig::EthStratumClient::setExtraNonce(const rapidjson::Value &nonce)
{
    if (!nonce.IsString()) {
        throw std::runtime_error("invalid mining.subscribe response: extra nonce is not a string");
    }

    const char *s = nonce.GetString();
    size_t len    = nonce.GetStringLength();

    // Skip "0x"
    if ((len >= 2) && (s[0] == '0') && (s[1] == 'x')) {
        s += 2;
        len -= 2;
    }

    if (len & 1) {
        throw std::runtime_error("invalid mining.subscribe response: extra nonce has an odd number of hex chars");
    }

    if (len > 8) {
        throw std::runtime_error("Invalid mining.subscribe response: extra nonce is too long");
    }

    std::string extra_nonce_str(s);
    extra_nonce_str.resize(16, '0');

    LOG_DEBUG("[%s] extra nonce set to %s", url(), s);

    m_extraNonce = { std::stoull(extra_nonce_str, nullptr, 16), s };
}


bool xmrig::EthStratumClient::isVextaStratum() const
{
    // LiquidPool publishes Vexta RandomX on 4513. The explicit algorithm
    // alias keeps the protocol usable on other Vexta-compatible pools.
    return m_pool.algorithm() == Algorithm::RX_VEXTA ||
           (m_pool.algorithm().family() == Algorithm::RANDOM_X && m_pool.port() == 4513);
}


const char *xmrig::EthStratumClient::errorMessage(const rapidjson::Value &error)
{
    if (error.IsArray() && error.GetArray().Size() > 1) {
        auto &value = error.GetArray()[1];
        if (value.IsString()) {
            return value.GetString();
        }
    }

    if (error.IsString()) {
        return error.GetString();
    }

    if (error.IsObject()) {
        return Json::getString(error, "message");
    }

    return nullptr;
}


void xmrig::EthStratumClient::oggLogin()
{
    using namespace rapidjson;

    Document doc(kObjectType);
    auto &allocator = doc.GetAllocator();
    Value params(kArrayType);
    params.PushBack(m_user.toJSON(), allocator);
    params.PushBack(m_password.toJSON(), allocator);

    JsonRequest::create(doc, m_sequence, "eth_submitLogin", params);
    send(doc, [this](const Value &result, bool success, uint64_t elapsed) {
        onOggLoginResponse(result, success, elapsed);
    });
}


void xmrig::EthStratumClient::onOggLoginResponse(const rapidjson::Value &result, bool success, uint64_t)
{
    if (!success || (result.IsBool() && !result.GetBool())) {
        const char *message = success ? "pool rejected eth_submitLogin" : errorMessage(result);
        LOG_ERR("%s " RED_BOLD("%s"), tag(), message ? message : "eth_submitLogin failed");
        close();
        return;
    }

    m_authorized = true;
    LOG_DEBUG("[%s] OggPoW eth-proxy login succeeded", url());
    m_listener->onLoginSuccess(this);
    requestOggWork();
}


void xmrig::EthStratumClient::requestOggWork()
{
    using namespace rapidjson;

    m_nextOggWork = Chrono::steadyMSecs() + 1000;

    Document doc(kObjectType);
    Value params(kArrayType);
    JsonRequest::create(doc, m_sequence, "eth_getWork", params);
    send(doc, [this](const Value &result, bool success, uint64_t elapsed) {
        onOggWorkResponse(result, success, elapsed);
    });
}


void xmrig::EthStratumClient::onOggWorkResponse(const rapidjson::Value &result, bool success, uint64_t)
{
    if (!success || !result.IsArray() || result.Size() < 3 ||
        !result[0].IsString() || !result[1].IsString() || !result[2].IsString()) {
        LOG_ERR("%s " RED("invalid eth_getWork response"), tag());
        return;
    }

    auto stripPrefix = [](const char *value) {
        return (value && value[0] == '0' && value[1] == 'x') ? value + 2 : value;
    };

    const char *header = stripPrefix(result[0].GetString());
    const char *seed = stripPrefix(result[1].GetString());
    const char *target = stripPrefix(result[2].GetString());

    // Job stores an Ethash-compatible 64-bit share target.  eth_getWork
    // returns a 256-bit boundary, so retain its least-significant 64 bits,
    // which is the compact target used by the GPU search kernel.
    std::string compactTarget(target ? target : "");
    if (compactTarget.size() > 16) {
        compactTarget = compactTarget.substr(compactTarget.size() - 16);
    }
    if (compactTarget.size() < 16) {
        compactTarget.insert(0, 16 - compactTarget.size(), '0');
    }

    Job job;
    job.setAlgorithm(m_pool.algorithm());
    job.setId(header);

    std::string blob(header ? header : "");
    blob += "00000000";
    if (!job.setBlob(blob.c_str()) || !job.setSeedHash(seed) || !job.setTarget(compactTarget.c_str())) {
        LOG_ERR("%s " RED("invalid OggPoW work data from eth_getWork"), tag());
        return;
    }

    if (result.Size() > 3) {
        if (result[3].IsUint64()) {
            job.setHeight(result[3].GetUint64());
        }
        else if (result[3].IsString()) {
            const char *height = result[3].GetString();
            job.setHeight(strtoull(height, nullptr, 0));
        }
    }

    bool ok = true;
    m_listener->onVerifyAlgorithm(this, job.algorithm(), &ok);
    if (!ok) {
        LOG_ERR("%s incompatible OggPoW algorithm received", tag());
        return;
    }

    if (m_job != job) {
        m_job = std::move(job);
        m_listener->onJobReceived(this, m_job, result);
    }
}


void xmrig::EthStratumClient::authorize()
{
    using namespace rapidjson;

    Document doc(kObjectType);
    auto &allocator = doc.GetAllocator();

    Value params(kArrayType);
    params.PushBack(m_user.toJSON(), allocator);
    params.PushBack(m_password.toJSON(), allocator);

    JsonRequest::create(doc, m_sequence, "mining.authorize", params);

    send(doc, [this](const rapidjson::Value& result, bool success, uint64_t elapsed) { onAuthorizeResponse(result, success, elapsed); });
}


void xmrig::EthStratumClient::onAuthorizeResponse(const rapidjson::Value &result, bool success, uint64_t)
{
    try {
        if (!success) {
            const auto message = errorMessage(result);
            if (message) {
                throw std::runtime_error(message);
            }

            throw std::runtime_error("mining.authorize call failed");
        }

        if (!result.IsBool()) {
            throw std::runtime_error("invalid mining.authorize response: result is not a boolean");
        }

        if (!result.GetBool()) {
            throw std::runtime_error("login failed");
        }
    } catch (const std::exception &ex) {
        LOG_ERR("%s " RED_BOLD("%s"), tag(), ex.what());

        close();
        return;
    }

    LOG_DEBUG("[%s] login succeeded", url());

    if (!m_authorized) {
        m_authorized = true;
        m_listener->onLoginSuccess(this);
    }
}


void xmrig::EthStratumClient::onSubscribeResponse(const rapidjson::Value &result, bool success, uint64_t)
{
    if (!success) {
        return;
    }

    try {
        if (!result.IsArray()) {
            throw std::runtime_error("invalid mining.subscribe response: result is not an array");
        }

        auto arr = result.GetArray();

        if (arr.Size() <= 1) {
            throw std::runtime_error("invalid mining.subscribe response: result array is too short");
        }

        setExtraNonce(arr[1]);

#       ifdef XMRIG_ALGO_GHOSTRIDER
        if ((arr.Size() > 2) && (arr[2].IsUint())) {
            m_extraNonce2Size = arr[2].GetUint();
        }
#       endif

        if (m_pool.isNicehash()) {
            using namespace rapidjson;
            Document doc(kObjectType);
            Value params(kArrayType);
            JsonRequest::create(doc, m_sequence, "mining.extranonce.subscribe", params);
            send(doc);
        }
    } catch (const std::exception &ex) {
        LOG_ERR("%s " RED("%s"), tag(), ex.what());

        m_extraNonce = { 0, {} };
    }
}


void xmrig::EthStratumClient::subscribe()
{
    using namespace rapidjson;

    Document doc(kObjectType);
    auto &allocator = doc.GetAllocator();

    Value params(kArrayType);
    params.PushBack(StringRef(agent()), allocator);

    JsonRequest::create(doc, m_sequence, "mining.subscribe", params);

    send(doc, [this](const rapidjson::Value& result, bool success, uint64_t elapsed) { onSubscribeResponse(result, success, elapsed); });
}
