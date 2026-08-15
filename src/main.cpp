#include "common.hpp"
#include "daemon_provider.hpp"
#include "hex.hpp"
#include "randomx_engine.hpp"
#include "stratum_provider.hpp"

#include <randomx.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <csignal>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace {

using namespace zqv;

constexpr const char* kFeeHost = ZQV_FEE_HOST;
constexpr const char* kFeePort = ZQV_FEE_PORT;
constexpr const char* kFeeUser = ZQV_FEE_USER;
constexpr const char* kFeePassword = "x";

std::atomic<bool> g_stop{false};

struct Config {
    Endpoint user_endpoint;
    std::string user;
    std::string password{"x"};
    unsigned threads{std::max(1u, std::thread::hardware_concurrency() / 2)};
    unsigned fee_percent{1};
    bool daemon{false};
    bool full_memory{true};
    unsigned runtime_seconds{0};
};

void print_usage() {
    std::cout
        << "Zerqavon Miner " ZERQAVON_MINER_VERSION "\n\n"
        << "Pool:   zerqavon-miner -o POOL:PORT -u WALLET_OR_USER [-p x] [-t N] [--fee 1]\n"
        << "Daemon: zerqavon-miner --daemon -o 127.0.0.1:37771 -u TESTNET_ADDRESS [-t N]\n\n"
        << "Options:\n"
        << "  -o, --url HOST:PORT       User pool or daemon endpoint\n"
        << "  -u, --user VALUE          Wallet address or pool username\n"
        << "  -p, --pass VALUE          Pool password (default: x)\n"
        << "  -t, --threads N           Mining threads\n"
        << "      --fee N               Fee mining percentage, 1..100 (default: 1)\n"
        << "      --daemon              Mine directly through daemon JSON-RPC\n"
        << "      --light               RandomX light mode (testing/low memory)\n"
        << "      --runtime N           Stop automatically after N seconds\n"
        << "  -h, --help                Show this help\n";
}

Config parse_config(int argc, char** argv) {
    Config config;
    std::string endpoint;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        const auto value = [&](const char* option) -> std::string {
            if (++i >= argc) throw std::runtime_error(std::string("missing value for ") + option);
            return argv[i];
        };
        if (arg == "-o" || arg == "--url") endpoint = value(arg.c_str());
        else if (arg == "-u" || arg == "--user") config.user = value(arg.c_str());
        else if (arg == "-p" || arg == "--pass") config.password = value(arg.c_str());
        else if (arg == "-t" || arg == "--threads") config.threads = static_cast<unsigned>(std::stoul(value(arg.c_str())));
        else if (arg == "--fee") config.fee_percent = static_cast<unsigned>(std::stoul(value(arg.c_str())));
        else if (arg == "--daemon") config.daemon = true;
        else if (arg == "--light") config.full_memory = false;
        else if (arg == "--runtime") config.runtime_seconds = static_cast<unsigned>(std::stoul(value(arg.c_str())));
        else if (arg == "-h" || arg == "--help") { print_usage(); std::exit(0); }
        else throw std::runtime_error("unknown option: " + arg);
    }
    if (endpoint.empty()) throw std::runtime_error("-o HOST:PORT is required");
    if (config.user.empty()) throw std::runtime_error("-u WALLET_OR_USER is required");
    if (config.threads < 1 || config.threads > 256) throw std::runtime_error("threads must be between 1 and 256");
    if (config.fee_percent < 1 || config.fee_percent > 100) throw std::runtime_error("fee must be between 1 and 100");
    config.user_endpoint = parse_endpoint(endpoint);
    return config;
}

class Miner {
public:
    explicit Miner(Config config)
        : config_(std::move(config)), contexts_(config_.threads, config_.full_memory),
          fee_(Source::Developer, Endpoint{kFeeHost, kFeePort}, kFeeUser, kFeePassword),
          started_(std::chrono::steady_clock::now()), fee_cycle_ms_(steady_milliseconds()) {
        if (config_.daemon) {
            user_ = std::make_unique<DaemonProvider>(config_.user_endpoint, config_.user);
        } else {
            user_ = std::make_unique<StratumProvider>(Source::User, config_.user_endpoint, config_.user, config_.password);
        }
    }

    void run() {
        print_banner();
        user_->start();
        fee_.start();
        for (unsigned i = 0; i < config_.threads; ++i) workers_.emplace_back(&Miner::worker, this, i);
        status_thread_ = std::thread(&Miner::status_loop, this);
        while (!g_stop.load()) {
            if (config_.runtime_seconds > 0 &&
                std::chrono::steady_clock::now() - started_ >= std::chrono::seconds(config_.runtime_seconds)) {
                g_stop.store(true);
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
        user_->stop();
        fee_.stop();
        for (auto& worker : workers_) worker.join();
        if (status_thread_.joinable()) status_thread_.join();
    }

private:
    void print_banner() const {
        std::cout << "Zerqavon Miner " ZERQAVON_MINER_VERSION << '\n'
                  << "User endpoint : " << user_->label() << '\n'
                  << "Algorithm     : ZQVXPOW v1 + RandomX\n"
                  << "Threads       : " << config_.threads << '\n'
                  << "RandomX mode  : " << (config_.full_memory ? "full" : "light") << '\n'
                  << "Fee mining    : " << config_.fee_percent << "%\n"
                  << "Failover      : 10 attempts, one per second\n\n";
    }

    bool fee_window() const {
        if (config_.fee_percent >= 100) return true;
        const auto elapsed = std::max<std::int64_t>(0, steady_milliseconds() - fee_cycle_ms_.load()) / 1000;
        return static_cast<unsigned>(elapsed % 100) >= 100 - config_.fee_percent;
    }

    static std::int64_t steady_milliseconds() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
    }

    std::pair<JobProvider*, std::shared_ptr<const Job>> select_job() {
        const bool user_ready = user_->connected() && user_->latest_job();
        const bool fee_ready = fee_.connected() && fee_.latest_job();

        if (!user_ready && user_->consecutive_failures() >= 10) {
            failover_active_.store(true);
            if (fee_ready) return {&fee_, fee_.latest_job()};
            return {nullptr, nullptr};
        }

        if (user_ready && failover_active_.exchange(false)) {
            fee_cycle_ms_.store(steady_milliseconds());
            std::cout << "[user] recovered; fee cycle restarted\n";
        }

        if (fee_window()) {
            if (fee_ready) return {&fee_, fee_.latest_job()};
            if (config_.fee_percent >= 100) return {nullptr, nullptr};
        }
        if (user_ready) return {user_.get(), user_->latest_job()};
        return {nullptr, nullptr};
    }

    void worker(unsigned index) {
        std::shared_ptr<const Job> current;
        std::shared_ptr<RandomXContext> context;
        std::vector<std::uint8_t> blob;
        std::uint32_t nonce = index;
        Source last_source = Source::User;
        std::uint64_t last_generation = 0;

        while (!g_stop.load()) {
            auto selected = select_job();
            if (!selected.first || !selected.second) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }
            current = std::move(selected.second);
            if (current->generation != last_generation || current->source != last_source) {
                try {
                    context = contexts_.get(*current);
                    blob = current->pow_blob;
                    nonce = index;
                    last_generation = current->generation;
                    last_source = current->source;
                } catch (const std::exception& error) {
                    std::cerr << "[randomx] " << error.what() << '\n';
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                    continue;
                }
            }

            if (current->pow_nonce_offset + 4 > blob.size()) {
                std::cerr << "[job] nonce offset outside blob\n";
                std::this_thread::sleep_for(std::chrono::seconds(1));
                continue;
            }
            std::array<std::uint8_t, 4> nonce_bytes{};
            for (unsigned byte = 0; byte < 4; ++byte) nonce_bytes[byte] = static_cast<std::uint8_t>(nonce >> (byte * 8));
            std::copy(nonce_bytes.begin(), nonce_bytes.end(), blob.begin() + static_cast<std::ptrdiff_t>(current->pow_nonce_offset));

            std::array<std::uint8_t, 32> hash{};
            randomx_calculate_hash(context->vm(index), blob.data(), blob.size(), hash.data());
            ++hashes_;
            try {
                if (hash_meets_job(hash, *current)) {
                    Share share{*current, nonce_bytes, hash};
                    const bool sent = selected.first->submit(share);
                    if (sent) {
                        const auto count = ++submitted_;
                        if (count <= 5 || count % 100 == 0) {
                            std::cout << '[' << source_name(current->source) << "] share/block submitted at height "
                                      << current->height << " (total " << count << ")\n";
                        }
                    } else {
                        ++rejected_;
                    }
                }
            } catch (const std::exception& error) {
                std::cerr << "[target] " << error.what() << '\n';
            }
            nonce += config_.threads;
        }
    }

    void status_loop() {
        auto previous_time = std::chrono::steady_clock::now();
        std::uint64_t previous_hashes = 0;
        while (!g_stop.load()) {
            for (int i = 0; i < 50 && !g_stop.load(); ++i) std::this_thread::sleep_for(std::chrono::milliseconds(200));
            const auto now = std::chrono::steady_clock::now();
            const auto total = hashes_.load();
            const double seconds = std::chrono::duration<double>(now - previous_time).count();
            const double speed = seconds > 0 ? (total - previous_hashes) / seconds : 0;
            const bool failover = !user_->connected() && user_->consecutive_failures() >= 10;
            std::cout << "[status] " << std::fixed << std::setprecision(1) << speed << " H/s, submitted "
                      << submitted_.load() << ", rejected " << rejected_.load()
                      << ", user failures " << user_->consecutive_failures()
                      << (failover ? ", fee failover active" : "") << '\n';
            previous_time = now;
            previous_hashes = total;
        }
    }

    Config config_;
    ContextManager contexts_;
    std::unique_ptr<JobProvider> user_;
    StratumProvider fee_;
    std::chrono::steady_clock::time_point started_;
    std::atomic<std::int64_t> fee_cycle_ms_;
    std::atomic<bool> failover_active_{false};
    std::vector<std::thread> workers_;
    std::thread status_thread_;
    std::atomic<std::uint64_t> hashes_{0};
    std::atomic<std::uint64_t> submitted_{0};
    std::atomic<std::uint64_t> rejected_{0};
};

void signal_handler(int) { g_stop.store(true); }

} // namespace

int main(int argc, char** argv) {
    try {
        std::signal(SIGINT, signal_handler);
        std::signal(SIGTERM, signal_handler);
        Miner miner(parse_config(argc, argv));
        miner.run();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << "\n\n";
        print_usage();
        return 1;
    }
}
