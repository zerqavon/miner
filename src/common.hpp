#pragma once

#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace zqv {

enum class Source { User, Developer };

struct Endpoint {
    std::string host;
    std::string port;
};

struct Job {
    Source source{Source::User};
    std::string id;
    std::string session_id;
    std::vector<std::uint8_t> pow_blob;
    std::vector<std::uint8_t> block_template;
    std::array<std::uint8_t, 32> seed{};
    std::string target_hex;
    std::uint64_t difficulty{0};
    std::uint64_t height{0};
    std::size_t pow_nonce_offset{0};
    std::size_t template_nonce_offset{0};
    std::uint64_t generation{0};
    bool daemon_job{false};
};

struct Share {
    Job job;
    std::array<std::uint8_t, 4> nonce{};
    std::array<std::uint8_t, 32> hash{};
};

class JobProvider {
public:
    virtual ~JobProvider() = default;
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual std::shared_ptr<const Job> latest_job() const = 0;
    virtual bool submit(const Share& share) = 0;
    virtual bool connected() const = 0;
    virtual unsigned consecutive_failures() const = 0;
    virtual std::string label() const = 0;
};

inline const char* source_name(Source source) {
    return source == Source::User ? "user" : "fee";
}

} // namespace zqv
