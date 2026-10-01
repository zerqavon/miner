/* XMRig
 * Copyright (c) 2019      Howard Chu  <https://github.com/hyc>
 * Copyright (c) 2018-2023 SChernykh   <https://github.com/SChernykh>
 * Copyright (c) 2016-2023 XMRig       <https://github.com/xmrig>, <support@xmrig.com>
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

#ifdef _MSC_VER
#pragma warning(disable:4244)
#endif

#include "net/Network.h"
#include "3rdparty/rapidjson/document.h"
#include "backend/common/Tags.h"
#include "base/io/log/Log.h"
#include "base/io/log/Tags.h"
#include "base/net/stratum/Client.h"
#include "base/net/stratum/NetworkState.h"
#include "base/net/stratum/SubmitResult.h"
#include "base/tools/Chrono.h"
#include "base/tools/Timer.h"
#include "core/config/Config.h"
#include "core/Controller.h"
#include "core/Miner.h"
#include "net/JobResult.h"
#include "net/JobResults.h"
#include "net/strategies/DonateStrategy.h"


#ifdef XMRIG_FEATURE_API
#   include "base/api/Api.h"
#   include "base/api/interfaces/IApiRequest.h"
#endif


#ifdef XMRIG_FEATURE_BENCHMARK
#   include "backend/common/benchmark/BenchState.h"
#endif


#include <algorithm>
#include <cinttypes>
#include <ctime>
#include <deque>
#include <iterator>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_set>


namespace {

std::mutex submittedSharesMutex;
std::deque<std::string> submittedSharesOrder;
std::unordered_set<std::string> submittedShares;

bool rememberSubmittedShare(const xmrig::JobResult &result)
{
    std::string key;
    key.reserve(result.jobId.size() + 32);
    key.append(1, static_cast<char>('0' + result.poolId));
    key.push_back('|');
    key.append(result.jobId.data());
    key.push_back('|');
    key.append(std::to_string(result.nonce));

    std::lock_guard<std::mutex> lock(submittedSharesMutex);

    if (!submittedShares.insert(key).second) {
        return false;
    }

    submittedSharesOrder.push_back(std::move(key));
    while (submittedSharesOrder.size() > 4096) {
        submittedShares.erase(submittedSharesOrder.front());
        submittedSharesOrder.pop_front();
    }

    return true;
}

} // namespace


xmrig::Network::Network(Controller *controller) :
    m_controller(controller)
{
    JobResults::setListener(this, controller->config()->cpu().isHwAES());
    controller->addListener(this);

#   ifdef XMRIG_FEATURE_API
    controller->api()->addListener(this);
#   endif

    m_state = new NetworkState(this);

    const Pools &pools = controller->config()->pools();
    m_strategy = pools.createStrategy(m_state);

    if (pools.donateLevel() > 0) {
        m_donate = new DonateStrategy(controller, this);
    }

    m_timer = new Timer(this, kTickInterval, kTickInterval);
}


xmrig::Network::~Network()
{
    JobResults::stop();

    delete m_timer;
    delete m_donate;
    delete m_strategy;
    delete m_state;
}


void xmrig::Network::connect()
{
    m_strategy->connect();
}


void xmrig::Network::execCommand(char command)
{
    switch (command) {
    case 's':
    case 'S':
        m_state->printResults();
        break;

    case 'c':
    case 'C':
        m_state->printConnection();
        break;

    default:
        break;
    }
}


void xmrig::Network::onActive(IStrategy *strategy, IClient *client)
{
    if (m_donate && m_donate == strategy) {
        LOG_NOTICE("%s " WHITE_BOLD("Liquid fee window started"), Tags::network());
        return;
    }

    const auto &pool = client->pool();

#   ifdef XMRIG_FEATURE_BENCHMARK
    if (pool.mode() == Pool::MODE_BENCHMARK) {
        return;
    }
#   endif

    char zmq_buf[32] = {};
    if (client->pool().zmq_port() >= 0) {
        snprintf(zmq_buf, sizeof(zmq_buf), " (ZMQ:%d)", client->pool().zmq_port());
    }

    const char *tlsVersion = client->tlsVersion();
    LOG_INFO("%s " WHITE_BOLD("use %s ") CYAN_BOLD("%s:%d%s ") GREEN_BOLD("%s") " " BLACK_BOLD("%s"),
             Tags::network(), client->mode(), pool.host().data(), pool.port(), zmq_buf, tlsVersion ? tlsVersion : "", client->ip().data());

    const char *fingerprint = client->tlsFingerprint();
    if (fingerprint != nullptr) {
        LOG_INFO("%s " BLACK_BOLD("fingerprint (SHA-256): \"%s\""), Tags::network(), fingerprint);
    }
}


void xmrig::Network::onConfigChanged(Config *config, Config *previousConfig)
{
    if (config->pools() == previousConfig->pools() || !config->pools().active()) {
        return;
    }

    m_strategy->stop();

    config->pools().print();

    delete m_strategy;
    m_strategy = config->pools().createStrategy(m_state);
    connect();
}


void xmrig::Network::onJob(IStrategy *strategy, IClient *client, const Job &job, const rapidjson::Value &)
{
    if (m_donate && m_donate->isActive() && m_donate != strategy) {
        return;
    }

    setJob(client, job, m_donate == strategy);
}


void xmrig::Network::onJobResult(const JobResult &result)
{
    if (result.index == 1 && m_donate) {
        m_donate->submit(result);
        return;
    }

    if (!rememberSubmittedShare(result)) {
        LOG_DEBUG("%s duplicate local share skipped pool %u job %s nonce %" PRIu64,
                  Tags::network(), result.poolId, result.jobId.data(), result.nonce);
        return;
    }

    JobResult submitResult = result;
    submitResult.switchOnAccept = m_controller->miner()->isMajorityPool(result.poolId);
    m_strategy->submit(submitResult);
}


void xmrig::Network::onLogin(IStrategy *, IClient *client, rapidjson::Document &doc, rapidjson::Value &params)
{
    using namespace rapidjson;
    auto &allocator = doc.GetAllocator();

    Algorithms algorithms     = m_controller->miner()->algorithms();
    const Algorithm algorithm = client->pool().algorithm();
    if (algorithm.isValid()) {
        const size_t index = static_cast<size_t>(std::distance(algorithms.begin(), std::find(algorithms.begin(), algorithms.end(), algorithm)));
        if (index > 0 && index < algorithms.size()) {
            std::swap(algorithms[0], algorithms[index]);
        }
    }

    Value algo(kArrayType);

    for (const auto &a : algorithms) {
        algo.PushBack(StringRef(a.name()), allocator);
    }

    params.AddMember("algo", algo, allocator);
}


void xmrig::Network::onPause(IStrategy *strategy)
{
    if (m_donate && m_donate == strategy) {
        LOG_NOTICE("%s " WHITE_BOLD("Liquid fee window finished"), Tags::network());
        m_strategy->resume();
    }

    if (!m_strategy->isActive()) {
        LOG_ERR("%s " RED("no active pools, stop mining"), Tags::network());

        return m_controller->miner()->pause();
    }
}


void xmrig::Network::onResultAccepted(IStrategy *, IClient *client, const SubmitResult &result, const char *error)
{
    uint64_t diff     = result.diff;
    const char *scale = NetworkState::scaleDiff(diff);

    if (error) {
        LOG_INFO("%s " RED_BOLD("Oops!") " (%" PRId64 "/%" PRId64 ") diff " WHITE_BOLD("%" PRIu64 "%s") " " RED("\"%s\"") " " BLACK_BOLD("(%" PRIu64 " ms)"),
                 backend_tag(result.backend), m_state->accepted(), m_state->rejected(), diff, scale, error, result.elapsed);
    }
    else {
        LOG_INFO("%s " GREEN_BOLD("Yay!") " (%" PRId64 "/%" PRId64 ") diff " WHITE_BOLD("%" PRIu64 "%s") " " BLACK_BOLD("(%" PRIu64 " ms)"),
                 backend_tag(result.backend), m_state->accepted(), m_state->rejected(), diff, scale, result.elapsed);

        if (client && result.switchOnAccept) {
            m_controller->miner()->switchPool(static_cast<uint8_t>(client->id()));
        }
    }
}


void xmrig::Network::onVerifyAlgorithm(IStrategy *, const IClient *, const Algorithm &algorithm, bool *ok)
{
    if (!m_controller->miner()->isEnabled(algorithm)) {
        *ok = false;

        return;
    }
}


#ifdef XMRIG_FEATURE_API
void xmrig::Network::onRequest(IApiRequest &request)
{
    if (request.type() == IApiRequest::REQ_SUMMARY) {
        request.accept();

        getResults(request.reply(), request.doc(), request.version());
        getConnection(request.reply(), request.doc(), request.version());
    }
}
#endif


void xmrig::Network::setJob(IClient *client, const Job &job, bool donate)
{
#   ifdef XMRIG_FEATURE_BENCHMARK
    if (!BenchState::size())
#   endif
    {
        uint64_t diff       = job.diff();
        const char *scale   = NetworkState::scaleDiff(diff);

        char zmq_buf[32] = {};
        if (client->pool().zmq_port() >= 0) {
            snprintf(zmq_buf, sizeof(zmq_buf), " (ZMQ:%d)", client->pool().zmq_port());
        }

        char tx_buf[32] = {};
        const uint32_t num_transactions = job.getNumTransactions();
        if (num_transactions > 0) {
            snprintf(tx_buf, sizeof(tx_buf), " (%u tx)", num_transactions);
        }

        char height_buf[64] = {};
        if (job.height() > 0) {
            snprintf(height_buf, sizeof(height_buf), " height " WHITE_BOLD("%" PRIu64), job.height());
        }

        if (donate) {
            LOG_INFO("%s " MAGENTA_BOLD("new fee job") " diff " WHITE_BOLD("%" PRIu64 "%s") " algo " WHITE_BOLD("%s") "%s%s",
                     Tags::network(), diff, scale, job.algorithm().name(), height_buf, tx_buf);
        }
        else {
            LOG_INFO("%s " MAGENTA_BOLD("new job") " from " WHITE_BOLD("%s:%d%s") " diff " WHITE_BOLD("%" PRIu64 "%s") " algo " WHITE_BOLD("%s") "%s%s",
                     Tags::network(), client->pool().host().data(), client->pool().port(), zmq_buf, diff, scale, job.algorithm().name(), height_buf, tx_buf);
        }
    }

    if (!donate && m_donate) {
        static_cast<DonateStrategy *>(m_donate)->update(client, job);
    }

    m_controller->miner()->setJob(job, donate);
}


void xmrig::Network::tick()
{
    const uint64_t now = Chrono::steadyMSecs();

    m_strategy->tick(now);

    if (m_donate) {
        m_donate->tick(now);
    }

#   ifdef XMRIG_FEATURE_API
    m_controller->api()->tick();
#   endif
}


#ifdef XMRIG_FEATURE_API
void xmrig::Network::getConnection(rapidjson::Value &reply, rapidjson::Document &doc, int version) const
{
    using namespace rapidjson;
    auto &allocator = doc.GetAllocator();

    reply.AddMember("algo",         m_state->algorithm().toJSON(), allocator);
    reply.AddMember("connection",   m_state->getConnection(doc, version), allocator);
}


void xmrig::Network::getResults(rapidjson::Value &reply, rapidjson::Document &doc, int version) const
{
    using namespace rapidjson;
    auto &allocator = doc.GetAllocator();

    reply.AddMember("results", m_state->getResults(doc, version), allocator);
}
#endif
