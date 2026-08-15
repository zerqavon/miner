#include "daemon_provider.hpp"
#include "hex.hpp"

#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include <chrono>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace zqv {
namespace {

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
using tcp = asio::ip::tcp;
using boost::property_tree::ptree;

std::string json_text(const ptree& tree) {
    std::ostringstream out;
    boost::property_tree::write_json(out, tree, false);
    return out.str();
}

ptree parse_json(const std::string& text) {
    ptree tree;
    std::istringstream in(text);
    boost::property_tree::read_json(in, tree);
    return tree;
}

} // namespace

DaemonProvider::DaemonProvider(Endpoint endpoint, std::string address)
    : endpoint_(std::move(endpoint)), address_(std::move(address)) {}

DaemonProvider::~DaemonProvider() { stop(); }

void DaemonProvider::start() {
    stopping_.store(false);
    thread_ = std::thread(&DaemonProvider::run, this);
}

void DaemonProvider::stop() {
    stopping_.store(true);
    if (thread_.joinable()) thread_.join();
}

std::shared_ptr<const Job> DaemonProvider::latest_job() const {
    std::lock_guard<std::mutex> lock(job_mutex_);
    return job_;
}

std::string DaemonProvider::label() const {
    return "daemon " + endpoint_.host + ":" + endpoint_.port;
}

std::string DaemonProvider::rpc_call(const std::string& body) const {
    asio::io_context io;
    tcp::resolver resolver(io);
    beast::tcp_stream stream(io);
    stream.expires_after(std::chrono::seconds(5));
    const auto endpoints = resolver.resolve(endpoint_.host, endpoint_.port);
    stream.connect(endpoints);

    http::request<http::string_body> request{http::verb::post, "/json_rpc", 11};
    request.set(http::field::host, endpoint_.host);
    request.set(http::field::user_agent, "ZerqavonMiner/" ZERQAVON_MINER_VERSION);
    request.set(http::field::content_type, "application/json");
    request.body() = body;
    request.prepare_payload();
    http::write(stream, request);

    beast::flat_buffer buffer;
    http::response<http::string_body> response;
    http::read(stream, buffer, response);
    beast::error_code ignored;
    stream.socket().shutdown(tcp::socket::shutdown_both, ignored);
    if (response.result_int() != 200) throw std::runtime_error("daemon HTTP status " + std::to_string(response.result_int()));
    return response.body();
}

void DaemonProvider::run() {
    while (!stopping_.load()) {
        try {
            ptree params;
            params.put("wallet_address", address_);
            params.put("reserve_size", 8);
            ptree request;
            request.put("jsonrpc", "2.0");
            request.put("id", "zqv");
            request.put("method", "get_block_template");
            request.add_child("params", params);
            const auto response = parse_json(rpc_call(json_text(request)));
            if (const auto error = response.get_optional<std::string>("error.message")) {
                throw std::runtime_error(*error);
            }

            const auto& result = response.get_child("result");
            auto next = std::make_shared<Job>();
            next->source = Source::User;
            next->id = "daemon-" + result.get<std::string>("height");
            next->pow_blob = from_hex(result.get<std::string>("blockhashing_blob"));
            next->block_template = from_hex(result.get<std::string>("blocktemplate_blob"));
            next->seed = array_from_hex<32>(result.get<std::string>("seed_hash"));
            next->difficulty = result.get<std::uint64_t>("difficulty");
            next->height = result.get<std::uint64_t>("height");
            if (!has_zqvx_pow_prefix(next->pow_blob)) throw std::runtime_error("daemon did not return a ZQVXPOW v1 blob");
            next->pow_nonce_offset = next->pow_blob.size() - 4;
            next->template_nonce_offset = canonical_nonce_offset(next->block_template);
            next->generation = ++generation_;
            next->daemon_job = true;

            {
                std::lock_guard<std::mutex> lock(job_mutex_);
                if (!job_ || job_->pow_blob != next->pow_blob) job_ = std::move(next);
            }
            connected_.store(true);
            failures_.store(0);
        } catch (const std::exception& error) {
            connected_.store(false);
            ++failures_;
            if (failures_.load() == 1 || failures_.load() == 10) {
                std::cerr << "[daemon] " << error.what() << " (attempt " << failures_.load() << ")\n";
            }
        }

        for (int i = 0; i < 10 && !stopping_.load(); ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}

bool DaemonProvider::submit(const Share& share) {
    try {
        auto blob = share.job.block_template;
        if (share.job.template_nonce_offset + share.nonce.size() > blob.size()) return false;
        std::copy(share.nonce.begin(), share.nonce.end(), blob.begin() + static_cast<std::ptrdiff_t>(share.job.template_nonce_offset));

        ptree params;
        ptree value;
        value.put("", to_hex(blob));
        params.push_back({"", value});
        ptree request;
        request.put("jsonrpc", "2.0");
        request.put("id", "submit");
        request.put("method", "submit_block");
        request.add_child("params", params);
        const auto response = parse_json(rpc_call(json_text(request)));
        if (const auto error = response.get_optional<std::string>("error.message")) {
            std::cerr << "[daemon] rejected: " << *error << '\n';
            return false;
        }
        const bool accepted = response.get<std::string>("result.status", "") == "OK";
        if (accepted) {
            std::lock_guard<std::mutex> lock(job_mutex_);
            if (job_ && job_->generation == share.job.generation) job_.reset();
        }
        return accepted;
    } catch (const std::exception& error) {
        std::cerr << "[daemon] submit failed: " << error.what() << '\n';
        return false;
    }
}

} // namespace zqv
