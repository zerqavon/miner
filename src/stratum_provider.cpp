#include "stratum_provider.hpp"
#include "hex.hpp"

#include <boost/asio/connect.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/write.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include <chrono>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace zqv {
namespace {

using boost::asio::ip::tcp;
using boost::property_tree::ptree;

std::string json_string(const std::string& value) {
    static constexpr char hex[] = "0123456789abcdef";
    std::string result;
    result.reserve(value.size() + 2);
    result.push_back('"');
    for (const auto raw : value) {
        const auto ch = static_cast<unsigned char>(raw);
        switch (ch) {
        case '"': result += "\\\""; break;
        case '\\': result += "\\\\"; break;
        case '\b': result += "\\b"; break;
        case '\f': result += "\\f"; break;
        case '\n': result += "\\n"; break;
        case '\r': result += "\\r"; break;
        case '\t': result += "\\t"; break;
        default:
            if (ch < 0x20) {
                result += "\\u00";
                result.push_back(hex[ch >> 4]);
                result.push_back(hex[ch & 0x0f]);
            } else {
                result.push_back(static_cast<char>(ch));
            }
        }
    }
    result.push_back('"');
    return result;
}

std::string login_request(const std::string& user, const std::string& password) {
    return "{\"id\":1,\"jsonrpc\":\"2.0\",\"method\":\"login\",\"params\":{\"login\":"
        + json_string(user) + ",\"pass\":" + json_string(password)
        + ",\"agent\":\"ZerqavonMiner/" ZERQAVON_MINER_VERSION "\"}}\n";
}

std::string submit_request(std::uint64_t id, const Share& share) {
    return "{\"id\":" + std::to_string(id)
        + ",\"jsonrpc\":\"2.0\",\"method\":\"submit\",\"params\":{\"id\":"
        + json_string(share.job.session_id) + ",\"job_id\":" + json_string(share.job.id)
        + ",\"nonce\":" + json_string(to_hex(share.nonce))
        + ",\"result\":" + json_string(to_hex(share.hash)) + "}}\n";
}

ptree parse_json(const std::string& text) {
    ptree tree;
    std::istringstream in(text);
    boost::property_tree::read_json(in, tree);
    return tree;
}

std::shared_ptr<tcp::socket> connect_with_timeout(boost::asio::io_context& io, const Endpoint& endpoint) {
    io.restart();
    auto socket = std::make_shared<tcp::socket>(io);
    tcp::resolver resolver(io);
    boost::asio::steady_timer timer(io);
    boost::system::error_code result = boost::asio::error::timed_out;
    bool complete = false;

    timer.expires_after(std::chrono::seconds(1));
    timer.async_wait([&](const boost::system::error_code& error) {
        if (!error && !complete) {
            resolver.cancel();
            boost::system::error_code ignored;
            socket->close(ignored);
        }
    });
    resolver.async_resolve(endpoint.host, endpoint.port,
        [&](const boost::system::error_code& error, const tcp::resolver::results_type& endpoints) {
            if (error) {
                result = error;
                complete = true;
                timer.cancel();
                return;
            }
            boost::asio::async_connect(*socket, endpoints,
                [&](const boost::system::error_code& connect_error, const tcp::endpoint&) {
                    result = connect_error;
                    complete = true;
                    timer.cancel();
                });
        });
    io.run();
    if (!complete || result) throw boost::system::system_error(result);
    return socket;
}

} // namespace

StratumProvider::StratumProvider(Source source, Endpoint endpoint, std::string user, std::string password)
    : source_(source), endpoint_(std::move(endpoint)), user_(std::move(user)), password_(std::move(password)) {}

StratumProvider::~StratumProvider() { stop(); }

void StratumProvider::start() {
    stopping_.store(false);
    thread_ = std::thread(&StratumProvider::run, this);
}

void StratumProvider::stop() {
    stopping_.store(true);
    {
        std::lock_guard<std::mutex> lock(socket_mutex_);
        if (socket_) {
            boost::system::error_code ignored;
            socket_->cancel(ignored);
            socket_->close(ignored);
        }
    }
    if (thread_.joinable()) thread_.join();
}

std::shared_ptr<const Job> StratumProvider::latest_job() const {
    std::lock_guard<std::mutex> lock(job_mutex_);
    return job_;
}

std::string StratumProvider::label() const {
    return std::string(source_name(source_)) + " pool " + endpoint_.host + ":" + endpoint_.port;
}

bool StratumProvider::send_line(const std::string& line) {
    std::lock_guard<std::mutex> lock(socket_mutex_);
    if (!socket_ || !socket_->is_open()) return false;
    boost::system::error_code error;
    boost::asio::write(*socket_, boost::asio::buffer(line), error);
    return !error;
}

void StratumProvider::run() {
    while (!stopping_.load()) {
        const auto attempt_started = std::chrono::steady_clock::now();
        try {
            auto socket = connect_with_timeout(io_, endpoint_);
            {
                std::lock_guard<std::mutex> lock(socket_mutex_);
                socket_ = socket;
            }

            if (!send_line(login_request(user_, password_))) throw std::runtime_error("login write failed");

            boost::asio::streambuf buffer;
            while (!stopping_.load()) {
                boost::system::error_code error;
                boost::asio::read_until(*socket, buffer, '\n', error);
                if (error) throw boost::system::system_error(error);
                std::istream input(&buffer);
                std::string line;
                std::getline(input, line);
                if (!line.empty()) parse_message(line);
            }
        } catch (const std::exception& error) {
            if (stopping_.load()) break;
            connected_.store(false);
            const auto attempt = ++failures_;
            if (attempt <= 10) {
                std::cerr << '[' << source_name(source_) << "] connection failed (" << attempt << "/10): " << error.what() << '\n';
            } else if (attempt % 30 == 0) {
                std::cerr << '[' << source_name(source_) << "] still reconnecting (attempt " << attempt << "): " << error.what() << '\n';
            }
        }

        {
            std::lock_guard<std::mutex> lock(socket_mutex_);
            if (socket_) {
                boost::system::error_code ignored;
                socket_->close(ignored);
                socket_.reset();
            }
        }
        while (!stopping_.load() && std::chrono::steady_clock::now() - attempt_started < std::chrono::seconds(1)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }
}

void StratumProvider::parse_message(const std::string& line) {
    const auto tree = parse_json(line);
    const auto response_id = tree.get<std::uint64_t>("id", 0);
    if (response_id > 1) {
        std::uint64_t height = 0;
        {
            std::lock_guard<std::mutex> lock(pending_mutex_);
            const auto pending = pending_submits_.find(response_id);
            if (pending == pending_submits_.end()) return;
            height = pending->second;
            pending_submits_.erase(pending);
        }
        if (const auto error = tree.get_optional<std::string>("error.message")) {
            const auto total = ++rejected_shares_;
            print_share_result(source_, false, height, total, *error);
        } else {
            const auto total = ++accepted_shares_;
            print_share_result(source_, true, height, total);
        }
        return;
    }

    if (const auto error = tree.get_optional<std::string>("error.message")) {
        std::cerr << '[' << source_name(source_) << "] server error: " << *error << '\n';
        return;
    }

    const auto method = tree.get<std::string>("method", "");
    if (method == "job") {
        publish_job(tree.get_child("params"), session_id_);
        return;
    }

    if (response_id == 1) {
        session_id_ = tree.get<std::string>("result.id", "");
        if (session_id_.empty()) throw std::runtime_error("pool login returned no session id");
        publish_job(tree.get_child("result.job"), session_id_);
        connected_.store(true);
        failures_.store(0);
        if (source_ == Source::User)
            std::cout << "[user] connected to " << endpoint_.host << ':' << endpoint_.port << '\n';
        else
            std::cout << "[fee] connected\n";
    }
}

void StratumProvider::publish_job(const ptree& tree, const std::string& session_id) {
    auto next = std::make_shared<Job>();
    next->source = source_;
    next->id = tree.get<std::string>("job_id");
    next->session_id = session_id;
    next->pow_blob = from_hex(tree.get<std::string>("blob"));
    next->seed = array_from_hex<32>(tree.get<std::string>("seed_hash"));
    next->target_hex = tree.get<std::string>("target");
    next->height = tree.get<std::uint64_t>("height", 0);
    next->generation = ++generation_;
    next->daemon_job = false;
    if (source_ == Source::User) {
        if (!has_zqvx_pow_prefix(next->pow_blob)) throw std::runtime_error("user pool job is not ZQVXPOW v1");
        next->pow_nonce_offset = next->pow_blob.size() - 4;
    } else {
        next->pow_nonce_offset = canonical_nonce_offset(next->pow_blob);
    }
    std::lock_guard<std::mutex> lock(job_mutex_);
    job_ = std::move(next);
}

bool StratumProvider::submit(const Share& share) {
    const auto id = ++submit_id_;
    {
        std::lock_guard<std::mutex> lock(pending_mutex_);
        pending_submits_[id] = share.job.height;
    }
    if (send_line(submit_request(id, share))) return true;
    {
        std::lock_guard<std::mutex> lock(pending_mutex_);
        pending_submits_.erase(id);
    }
    const auto total = ++rejected_shares_;
    print_share_result(source_, false, share.job.height, total, "submission write failed");
    return false;
}

} // namespace zqv
