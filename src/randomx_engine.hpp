#pragma once

#include "common.hpp"

#include <randomx.h>

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace zqv {

class RandomXContext {
public:
    RandomXContext(const std::array<std::uint8_t, 32>& seed, unsigned workers, bool full_memory);
    ~RandomXContext();
    RandomXContext(const RandomXContext&) = delete;
    RandomXContext& operator=(const RandomXContext&) = delete;

    randomx_vm* vm(unsigned worker) const { return vms_.at(worker % vms_.size()); }
    bool full_memory() const { return dataset_ != nullptr; }

private:
    randomx_flags flags_{RANDOMX_FLAG_DEFAULT};
    randomx_cache* cache_{nullptr};
    randomx_dataset* dataset_{nullptr};
    std::vector<randomx_vm*> vms_;
};

class ContextManager {
public:
    ContextManager(unsigned workers, bool full_user_memory) : workers_(workers), full_user_memory_(full_user_memory) {}
    std::shared_ptr<RandomXContext> get(const Job& job);

private:
    unsigned workers_;
    bool full_user_memory_;
    std::mutex mutex_;
    std::map<Source, std::pair<std::string, std::shared_ptr<RandomXContext>>> contexts_;
};

bool hash_meets_job(const std::array<std::uint8_t, 32>& hash, const Job& job);

} // namespace zqv
