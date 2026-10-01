#ifndef XMRIG_ROUNDROBINSTRATEGY_H
#define XMRIG_ROUNDROBINSTRATEGY_H

#include <vector>

#include "base/kernel/interfaces/IClientListener.h"
#include "base/kernel/interfaces/IStrategy.h"
#include "base/net/stratum/Pool.h"
#include "base/tools/Object.h"

namespace xmrig {

class IStrategyListener;

class RoundRobinStrategy : public IStrategy, public IClientListener
{
public:
    XMRIG_DISABLE_COPY_MOVE_DEFAULT(RoundRobinStrategy)

    RoundRobinStrategy(const std::vector<Pool> &pools, int retryPause, int retries, IStrategyListener *listener);
    ~RoundRobinStrategy() override;

protected:
    bool isActive() const override;
    IClient *client() const override;
    int64_t submit(const JobResult &result) override;
    void connect() override;
    void resume() override;
    void setAlgo(const Algorithm &algo) override;
    void setProxy(const ProxyUrl &proxy) override;
    void stop() override;
    void tick(uint64_t now) override;

    void onClose(IClient *client, int failures) override;
    void onJobReceived(IClient *client, const Job &job, const rapidjson::Value &params) override;
    void onLogin(IClient *client, rapidjson::Document &doc, rapidjson::Value &params) override;
    void onLoginSuccess(IClient *client) override;
    void onResultAccepted(IClient *client, const SubmitResult &result, const char *error) override;
    void onVerifyAlgorithm(const IClient *client, const Algorithm &algorithm, bool *ok) override;

private:
    void switchPool();

    IStrategyListener *m_listener;
    std::vector<IClient*> m_pools;
    int m_active = -1;
    bool m_stopping = false;
};

}

#endif
