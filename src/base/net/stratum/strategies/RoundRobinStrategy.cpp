#include "base/net/stratum/strategies/RoundRobinStrategy.h"

#include "3rdparty/rapidjson/document.h"
#include "base/kernel/interfaces/IClient.h"
#include "base/kernel/interfaces/IStrategyListener.h"
#include "base/net/stratum/Job.h"
#include "net/JobResult.h"

xmrig::RoundRobinStrategy::RoundRobinStrategy(const std::vector<Pool> &pools, int retryPause, int retries, IStrategyListener *listener) :
    m_listener(listener)
{
    for (const auto &pool : pools) {
        if (!pool.isEnabled()) {
            continue;
        }

        auto *client = pool.createClient(static_cast<int>(m_pools.size()), this);
        client->setRetries(retries);
        client->setRetryPause(retryPause * 1000);
        m_pools.push_back(client);
    }
}

xmrig::RoundRobinStrategy::~RoundRobinStrategy()
{
    for (auto *client : m_pools) {
        client->deleteLater();
    }
}

bool xmrig::RoundRobinStrategy::isActive() const
{
    return m_active >= 0 && m_active < static_cast<int>(m_pools.size());
}

xmrig::IClient *xmrig::RoundRobinStrategy::client() const
{
    return isActive() ? m_pools[static_cast<size_t>(m_active)] : nullptr;
}

int64_t xmrig::RoundRobinStrategy::submit(const JobResult &result)
{
    if (result.poolId < m_pools.size()) {
        return m_pools[result.poolId]->submit(result);
    }

    return isActive() ? client()->submit(result) : -1;
}

void xmrig::RoundRobinStrategy::connect()
{
    for (auto *pool : m_pools) {
        pool->connect();
    }
}

void xmrig::RoundRobinStrategy::resume()
{
    for (auto *pool : m_pools) {
        if (pool->job().isValid()) {
            Job job = pool->job();
            job.setPoolId(static_cast<uint8_t>(pool->id()));
            m_listener->onJob(this, pool, job, rapidjson::Value(rapidjson::kNullType));
        }
    }
}

void xmrig::RoundRobinStrategy::setAlgo(const Algorithm &algo)
{
    for (auto *pool : m_pools) {
        if (!pool->pool().algorithm().isValid()) {
            pool->setAlgo(algo);
        }
    }
}

void xmrig::RoundRobinStrategy::setProxy(const ProxyUrl &proxy)
{
    for (auto *pool : m_pools) {
        pool->setProxy(proxy);
    }
}

void xmrig::RoundRobinStrategy::stop()
{
    m_stopping = true;
    for (auto *pool : m_pools) {
        pool->disconnect();
    }
    m_active = -1;
}

void xmrig::RoundRobinStrategy::tick(uint64_t now)
{
    for (auto *pool : m_pools) {
        pool->tick(now);
    }
}

void xmrig::RoundRobinStrategy::onClose(IClient *client, int failures)
{
    if (m_stopping || failures == -1) {
        return;
    }
}

void xmrig::RoundRobinStrategy::onJobReceived(IClient *client, const Job &job, const rapidjson::Value &params)
{
    Job poolJob = job;
    poolJob.setPoolId(static_cast<uint8_t>(client->id()));
    m_listener->onJob(this, client, poolJob, params);
}

void xmrig::RoundRobinStrategy::onLogin(IClient *client, rapidjson::Document &doc, rapidjson::Value &params)
{
    m_listener->onLogin(this, client, doc, params);
}

void xmrig::RoundRobinStrategy::onLoginSuccess(IClient *client)
{
    if (!isActive()) {
        m_active = client->id();
        m_listener->onActive(this, client);
        if (client->job().isValid()) {
            Job job = client->job();
            job.setPoolId(static_cast<uint8_t>(client->id()));
            m_listener->onJob(this, client, job, rapidjson::Value(rapidjson::kNullType));
        }
    }
}

void xmrig::RoundRobinStrategy::onResultAccepted(IClient *client, const SubmitResult &result, const char *error)
{
    m_listener->onResultAccepted(this, client, result, error);
}

void xmrig::RoundRobinStrategy::onVerifyAlgorithm(const IClient *client, const Algorithm &algorithm, bool *ok)
{
    m_listener->onVerifyAlgorithm(this, client, algorithm, ok);
}

void xmrig::RoundRobinStrategy::switchPool()
{
    if (m_pools.size() < 2) {
        return;
    }

    const int next = (m_active + 1) % static_cast<int>(m_pools.size());
    if (!m_pools[static_cast<size_t>(next)]->job().isValid()) {
        return;
    }

    m_active = next;
    Job job = client()->job();
    job.setPoolId(static_cast<uint8_t>(client()->id()));
    m_listener->onJob(this, client(), job, rapidjson::Value(rapidjson::kNullType));
}
