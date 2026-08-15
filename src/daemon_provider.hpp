#pragma once

#include "common.hpp"

#include <thread>

namespace zqv {

class DaemonProvider final : public JobProvider {
public:
    DaemonProvider(Endpoint endpoint, std::string address);
    ~DaemonProvider() override;

    void start() override;
    void stop() override;
    std::shared_ptr<const Job> latest_job() const override;
    bool submit(const Share& share) override;
    bool connected() const override { return connected_.load(); }
    unsigned consecutive_failures() const override { return failures_.load(); }
    std::uint64_t accepted_shares() const override { return accepted_shares_.load(); }
    std::uint64_t rejected_shares() const override { return rejected_shares_.load(); }
    std::string label() const override;

private:
    std::string rpc_call(const std::string& body) const;
    void run();

    Endpoint endpoint_;
    std::string address_;
    std::atomic<bool> stopping_{false};
    std::atomic<bool> connected_{false};
    std::atomic<unsigned> failures_{0};
    std::thread thread_;
    mutable std::mutex job_mutex_;
    std::shared_ptr<const Job> job_;
    std::atomic<std::uint64_t> generation_{0};
    std::atomic<std::uint64_t> accepted_shares_{0};
    std::atomic<std::uint64_t> rejected_shares_{0};
};

} // namespace zqv

