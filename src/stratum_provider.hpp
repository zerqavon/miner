#pragma once

#include "common.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/property_tree/ptree.hpp>

#include <thread>

namespace zqv {

class StratumProvider final : public JobProvider {
public:
    StratumProvider(Source source, Endpoint endpoint, std::string user, std::string password);
    ~StratumProvider() override;

    void start() override;
    void stop() override;
    std::shared_ptr<const Job> latest_job() const override;
    bool submit(const Share& share) override;
    bool connected() const override { return connected_.load(); }
    unsigned consecutive_failures() const override { return failures_.load(); }
    std::string label() const override;

private:
    void run();
    void parse_message(const std::string& line);
    void publish_job(const boost::property_tree::ptree& tree, const std::string& session_id);
    bool send_line(const std::string& line);

    Source source_;
    Endpoint endpoint_;
    std::string user_;
    std::string password_;
    std::atomic<bool> stopping_{false};
    std::atomic<bool> connected_{false};
    std::atomic<unsigned> failures_{0};
    std::atomic<std::uint64_t> generation_{0};
    std::atomic<std::uint64_t> submit_id_{10};
    std::thread thread_;
    mutable std::mutex job_mutex_;
    std::shared_ptr<const Job> job_;
    mutable std::mutex socket_mutex_;
    boost::asio::io_context io_;
    std::shared_ptr<boost::asio::ip::tcp::socket> socket_;
    std::string session_id_;
};

} // namespace zqv
