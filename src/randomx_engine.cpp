#include "randomx_engine.hpp"
#include "hex.hpp"

#include <boost/multiprecision/cpp_int.hpp>

#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <thread>

namespace zqv {
namespace {

using boost::multiprecision::cpp_int;

cpp_int little_integer(const std::uint8_t* bytes, std::size_t size) {
    cpp_int value = 0;
    for (std::size_t i = 0; i < size; ++i) value += cpp_int(bytes[i]) << (i * 8);
    return value;
}

} // namespace

RandomXContext::RandomXContext(const std::array<std::uint8_t, 32>& seed, unsigned workers, bool full_memory) {
    flags_ = randomx_get_flags();
    cache_ = randomx_alloc_cache(flags_);
    if (!cache_) {
        flags_ = static_cast<randomx_flags>(flags_ & ~RANDOMX_FLAG_JIT);
        cache_ = randomx_alloc_cache(flags_);
    }
    if (!cache_) throw std::runtime_error("RandomX cache allocation failed");
    randomx_init_cache(cache_, seed.data(), seed.size());

    if (full_memory) {
        dataset_ = randomx_alloc_dataset(RANDOMX_FLAG_DEFAULT);
        if (!dataset_) throw std::runtime_error("RandomX dataset allocation failed");
        const auto count = randomx_dataset_item_count();
        const unsigned builders = std::max(1u, std::min(workers, std::thread::hardware_concurrency()));
        std::vector<std::thread> threads;
        for (unsigned i = 0; i < builders; ++i) {
            const unsigned long begin = count * i / builders;
            const unsigned long end = count * (i + 1) / builders;
            threads.emplace_back([this, begin, end] { randomx_init_dataset(dataset_, cache_, begin, end - begin); });
        }
        for (auto& thread : threads) thread.join();
        flags_ = static_cast<randomx_flags>(flags_ | RANDOMX_FLAG_FULL_MEM);
    }

    for (unsigned i = 0; i < std::max(1u, workers); ++i) {
        auto* vm = randomx_create_vm(flags_, dataset_ ? nullptr : cache_, dataset_);
        if (!vm) throw std::runtime_error("RandomX VM allocation failed");
        vms_.push_back(vm);
    }
    if (dataset_ && cache_) {
        randomx_release_cache(cache_);
        cache_ = nullptr;
    }
}

RandomXContext::~RandomXContext() {
    for (auto* vm : vms_) randomx_destroy_vm(vm);
    if (dataset_) randomx_release_dataset(dataset_);
    if (cache_) randomx_release_cache(cache_);
}

std::shared_ptr<RandomXContext> ContextManager::get(const Job& job) {
    const std::string key = to_hex(job.seed);
    std::lock_guard<std::mutex> lock(mutex_);
    auto& slot = contexts_[job.source];
    if (slot.first == key && slot.second) return slot.second;
    const bool full_memory = job.source == Source::User && full_user_memory_;
    std::cout << "[randomx] initializing " << (full_memory ? "full" : "light")
              << " context for " << source_name(job.source) << " seed "
              << to_hex(job.seed).substr(0, 16) << "...\n";
    auto context = std::make_shared<RandomXContext>(job.seed, workers_, full_memory);
    slot = {key, context};
    return context;
}

bool hash_meets_job(const std::array<std::uint8_t, 32>& hash, const Job& job) {
    const cpp_int hash_value = little_integer(hash.data(), hash.size());
    if (job.difficulty > 0) {
        const cpp_int maximum = (cpp_int(1) << 256) - 1;
        return hash_value * job.difficulty <= maximum;
    }

    const auto target_bytes = from_hex(job.target_hex);
    if (target_bytes.size() == 32) return hash_value <= little_integer(target_bytes.data(), target_bytes.size());
    if (target_bytes.size() == 4 || target_bytes.size() == 8) {
        const cpp_int target = little_integer(target_bytes.data(), target_bytes.size());
        if (target == 0) return false;
        const cpp_int word_max = (cpp_int(1) << (target_bytes.size() * 8)) - 1;
        const cpp_int difficulty = word_max / target;
        const cpp_int maximum = (cpp_int(1) << 256) - 1;
        return hash_value * difficulty <= maximum;
    }
    throw std::runtime_error("unsupported pool target size");
}

} // namespace zqv
