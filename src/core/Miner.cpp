/* XMRig
 * Copyright (c) 2018-2021 SChernykh   <https://github.com/SChernykh>
 * Copyright (c) 2016-2021 XMRig       <https://github.com/xmrig>, <support@xmrig.com>
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <algorithm>
#include <array>
#include <mutex>
#include <thread>


#include "core/Miner.h"
#include "core/Taskbar.h"
#include "3rdparty/rapidjson/document.h"
#include "backend/common/Hashrate.h"
#include "backend/cpu/Cpu.h"
#include "backend/cpu/CpuBackend.h"
#include "base/io/log/Log.h"
#include "base/io/log/Tags.h"
#include "base/kernel/Platform.h"
#include "base/net/stratum/Job.h"
#include "base/tools/Object.h"
#include "base/tools/Timer.h"
#include "core/config/Config.h"
#include "core/Controller.h"
#include "crypto/common/Nonce.h"
#include "version.h"


#ifdef XMRIG_FEATURE_HWLOC
#   include <hwloc.h>
#endif

#ifdef XMRIG_FEATURE_API
#   include "base/api/Api.h"
#   include "base/api/interfaces/IApiRequest.h"
#endif


#ifdef XMRIG_FEATURE_OPENCL
#   include "backend/opencl/OclBackend.h"
#   include "backend/opencl/OclConfig.h"
#endif


#ifdef XMRIG_FEATURE_CUDA
#   include "backend/cuda/CudaBackend.h"
#   include "backend/cuda/CudaConfig.h"
#endif


#ifdef XMRIG_ALGO_RANDOMX
#   include "crypto/rx/Profiler.h"
#   include "crypto/rx/Rx.h"
#   include "crypto/rx/RxConfig.h"
#   include "crypto/rx/RxAlgo.h"
#endif


#ifdef XMRIG_ALGO_GHOSTRIDER
#   include "crypto/ghostrider/ghostrider.h"
#endif


namespace xmrig {


static std::mutex mutex;


class MinerPrivate
{
public:
    XMRIG_DISABLE_COPY_MOVE_DEFAULT(MinerPrivate)


    inline explicit MinerPrivate(Controller *controller) : controller(controller) {}


    inline ~MinerPrivate()
    {
        delete timer;

        for (IBackend *backend : backends) {
            delete backend;
        }

#       ifdef XMRIG_ALGO_RANDOMX
        Rx::destroy();
#       endif
    }


    bool isEnabled(const Algorithm &algorithm) const
    {
        for (IBackend *backend : backends) {
            if (backend->isEnabled() && backend->isEnabled(algorithm)) {
                return true;
            }
        }

        return false;
    }


    inline void rebuild()
    {
        algorithms = Algorithm::all([this](const Algorithm &algo) { return isEnabled(algo); });
    }


    inline bool isDualPool() const
    {
        return controller->config()->pools().active() == 2;
    }


    inline bool isGpuEnabled() const
    {
#       ifdef XMRIG_FEATURE_OPENCL
        if (controller->config()->cl().isEnabled()) {
            return true;
        }
#       endif

#       ifdef XMRIG_FEATURE_CUDA
        if (controller->config()->cuda().isEnabled()) {
            return true;
        }
#       endif

        return false;
    }


    inline bool isDualGpuSplit() const
    {
        return isDualPool() && isGpuEnabled();
    }


    inline bool isDualCpuSplit() const
    {
        return isDualPool()
               && !isDualGpuSplit()
               && poolJobs[0].isValid()
               && poolJobs[1].isValid()
               && !isDualAlgorithmCompatible(poolJobs[0].algorithm(), poolJobs[1].algorithm());
    }


    inline Job jobForBackend(size_t backendIndex)
    {
        if (job.index() == 1 || !isDualGpuSplit()) {
            return job;
        }

        const size_t poolId = backendIndex == 0 ? 0 : 1;
        if (poolJobs[poolId].isValid()) {
            return poolJobs[poolId];
        }

        return job;
    }


    inline void handleJobChange()
    {
        if (!enabled) {
            Nonce::pause(true);
        }

        if (reset) {
            Nonce::reset(job.index());
        }

        for (size_t i = 0; i < backends.size(); ++i) {
            backends[i]->setJob(jobForBackend(i));
        }

        Nonce::touch();

        if (active && enabled) {
            Nonce::pause(false);
        }

        if (ticks == 0) {
            ticks++;
            timer->start(500, 500);
        }
    }


    inline bool isSamePhysicalCore(int64_t a, int64_t b) const
    {
        if (a < 0 || b < 0) {
            return false;
        }

#       ifdef XMRIG_FEATURE_HWLOC
        const auto topology = Cpu::info()->topology();
        const auto coreForPu = [topology](int64_t affinity) -> hwloc_obj_t {
            auto obj = hwloc_get_pu_obj_by_os_index(topology, static_cast<unsigned>(affinity));
            while (obj && obj->type != HWLOC_OBJ_CORE) {
                obj = obj->parent;
            }

            return obj;
        };

        const auto coreA = coreForPu(a);
        const auto coreB = coreForPu(b);

        return coreA && coreA == coreB;
#       else
        return a == b;
#       endif
    }


    inline bool isMinorityWorker(size_t workerId, int64_t affinity)
    {
        if (workerId == 0 && affinity >= 0) {
            minorityCoreAffinity = affinity;
        }

        if (minorityCoreAffinity >= 0) {
            return isSamePhysicalCore(affinity, minorityCoreAffinity);
        }

        const size_t cores = Cpu::info()->cores();
        const size_t logical = Cpu::info()->threads();
        const size_t threadsPerCore = cores > 0 ? std::max<size_t>(logical / cores, 1) : 1;

        return workerId < threadsPerCore;
    }


    inline Job jobForWorker(size_t workerId, int64_t affinity)
    {
        if (isDualGpuSplit() && poolJobs[0].isValid()) {
            return poolJobs[0];
        }

        if (isDualCpuSplit()) {
            return job;
        }

        if (!isDualPool() || activePool < 0 || !poolJobs[0].isValid() || !poolJobs[1].isValid()) {
            return job;
        }

        const uint8_t poolId = isMinorityWorker(workerId, affinity) ? static_cast<uint8_t>(1 - activePool) : static_cast<uint8_t>(activePool);

        return poolJobs[poolId];
    }


    inline Job jobForWorker(size_t workerId, int64_t affinity, int8_t poolId)
    {
        if (poolId >= 0 && isDualCpuSplit() && static_cast<size_t>(poolId) < poolJobs.size() && poolJobs[static_cast<size_t>(poolId)].isValid()) {
            return poolJobs[static_cast<size_t>(poolId)];
        }

        return jobForWorker(workerId, affinity);
    }


    static inline bool isDualAlgorithmCompatible(const Algorithm &a, const Algorithm &b)
    {
#       ifdef XMRIG_ALGO_RANDOMX
        return a.family() == Algorithm::RANDOM_X && b.family() == Algorithm::RANDOM_X && a.l3() == b.l3() && RxAlgo::base(a) == RxAlgo::base(b);
#       else
        return a.id() == b.id() && a.l3() == b.l3();
#       endif
    }


#   ifdef XMRIG_FEATURE_API
    void getMiner(rapidjson::Value &reply, rapidjson::Document &doc, int) const
    {
        using namespace rapidjson;
        auto &allocator = doc.GetAllocator();

        reply.AddMember("version",      APP_VERSION, allocator);
        reply.AddMember("kind",         APP_KIND, allocator);
        reply.AddMember("ua",           Platform::userAgent().toJSON(), allocator);
        reply.AddMember("cpu",          Cpu::toJSON(doc), allocator);
        reply.AddMember("donate_level", controller->config()->pools().donateLevel(), allocator);
        reply.AddMember("paused",       !enabled, allocator);

        Value algo(kArrayType);

        for (const Algorithm &a : algorithms) {
            algo.PushBack(StringRef(a.name()), allocator);
        }

        reply.AddMember("algorithms", algo, allocator);
    }


    void getHashrate(rapidjson::Value &reply, rapidjson::Document &doc, int version) const
    {
        using namespace rapidjson;
        auto &allocator = doc.GetAllocator();

        Value hashrate(kObjectType);
        Value total(kArrayType);
        Value threads(kArrayType);

        std::pair<bool, double> t[3] = { { true, 0.0 }, { true, 0.0 }, { true, 0.0 } };

        for (IBackend *backend : backends) {
            const Hashrate *hr = backend->hashrate();
            if (!hr) {
                continue;
            }

            const auto h0 = hr->calc(Hashrate::ShortInterval);
            const auto h1 = hr->calc(Hashrate::MediumInterval);
            const auto h2 = hr->calc(Hashrate::LargeInterval);

            if (h0.first) { t[0].second += h0.second; } else { t[0].first = false; }
            if (h1.first) { t[1].second += h1.second; } else { t[1].first = false; }
            if (h2.first) { t[2].second += h2.second; } else { t[2].first = false; }

            if (version > 1) {
                continue;
            }

            for (size_t i = 0; i < hr->threads(); i++) {
                Value thread(kArrayType);
                thread.PushBack(Hashrate::normalize(hr->calc(i, Hashrate::ShortInterval)),  allocator);
                thread.PushBack(Hashrate::normalize(hr->calc(i, Hashrate::MediumInterval)), allocator);
                thread.PushBack(Hashrate::normalize(hr->calc(i, Hashrate::LargeInterval)),  allocator);

                threads.PushBack(thread, allocator);
            }
        }

        total.PushBack(Hashrate::normalize(t[0]),  allocator);
        total.PushBack(Hashrate::normalize(t[1]), allocator);
        total.PushBack(Hashrate::normalize(t[2]),  allocator);

        hashrate.AddMember("total",   total, allocator);
        hashrate.AddMember("highest", Hashrate::normalize({ maxHashrate[algorithm] > 0.0, maxHashrate[algorithm] }), allocator);

        if (version == 1) {
            hashrate.AddMember("threads", threads, allocator);
        }

        reply.AddMember("hashrate", hashrate, allocator);
    }


    void getBackends(rapidjson::Value &reply, rapidjson::Document &doc) const
    {
        using namespace rapidjson;
        auto &allocator = doc.GetAllocator();

        reply.SetArray();

        for (IBackend *backend : backends) {
            reply.PushBack(backend->toJSON(doc), allocator);
        }
    }
#   endif


    static inline void printProfile()
    {
#       ifdef XMRIG_FEATURE_PROFILING
        ProfileScopeData* data[ProfileScopeData::MAX_DATA_COUNT];

        const uint32_t n = std::min<uint32_t>(ProfileScopeData::s_dataCount, ProfileScopeData::MAX_DATA_COUNT);
        memcpy(data, ProfileScopeData::s_data, n * sizeof(ProfileScopeData*));

        std::sort(data, data + n, [](ProfileScopeData* a, ProfileScopeData* b) {
            return strcmp(a->m_threadId, b->m_threadId) < 0;
        });

        std::map<std::string, std::pair<uint32_t, double>> averageTime;

        for (uint32_t i = 0; i < n;)
        {
            uint32_t n1 = i;
            while ((n1 < n) && (strcmp(data[i]->m_threadId, data[n1]->m_threadId) == 0)) {
                ++n1;
            }

            std::sort(data + i, data + n1, [](ProfileScopeData* a, ProfileScopeData* b) {
                return a->m_totalCycles > b->m_totalCycles;
            });

            for (uint32_t j = i; j < n1; ++j) {
                ProfileScopeData* p = data[j];
                const double t = p->m_totalCycles / p->m_totalSamples * 1e9 / ProfileScopeData::s_tscSpeed;
                LOG_INFO("%s Thread %6s | %-30s | %7.3f%% | %9.0f ns",
                    Tags::profiler(),
                    p->m_threadId,
                    p->m_name,
                    p->m_totalCycles * 100.0 / data[i]->m_totalCycles,
                    t
                );
                auto& value = averageTime[p->m_name];
                ++value.first;
                value.second += t;
            }

            LOG_INFO("%s --------------|--------------------------------|----------|-------------", Tags::profiler());

            i = n1;
        }

        for (auto& data : averageTime) {
            LOG_INFO("%s %-30s %9.1f ns", Tags::profiler(), data.first.c_str(), data.second.second / data.second.first);
        }
#       endif
    }


    void printHashrate(bool details)
    {
        char num[16 * 5] = { 0 };
        std::pair<bool, double> speed[3] = { { true, 0.0 }, { true, 0.0 }, { true, 0.0 } };
        uint32_t count   = 0;

        double avg_hashrate = 0.0;

        for (auto backend : backends) {
            const auto hashrate = backend->hashrate();
            if (hashrate) {
                ++count;

                const auto h0 = hashrate->calc(Hashrate::ShortInterval);
                const auto h1 = hashrate->calc(Hashrate::MediumInterval);
                const auto h2 = hashrate->calc(Hashrate::LargeInterval);

                if (h0.first) { speed[0].second += h0.second; } else { speed[0].first = false; }
                if (h1.first) { speed[1].second += h1.second; } else { speed[1].first = false; }
                if (h2.first) { speed[2].second += h2.second; } else { speed[2].first = false; }

                avg_hashrate += hashrate->average();
            }

            backend->printHashrate(details);
        }

        if (!count) {
            return;
        }

        printProfile();

        double scale  = 1.0;
        const char* h = "H/s";

        if ((speed[0].second >= 1e6) || (speed[1].second >= 1e6) || (speed[2].second >= 1e6) || (maxHashrate[algorithm] >= 1e6)) {
            scale = 1e-6;

            speed[0].second *= scale;
            speed[1].second *= scale;
            speed[2].second *= scale;

            h = "MH/s";
        }

        char avg_hashrate_buf[64];
        avg_hashrate_buf[0] = '\0';

#       ifdef XMRIG_ALGO_GHOSTRIDER
        if (algorithm.family() == Algorithm::GHOSTRIDER) {
            snprintf(avg_hashrate_buf, sizeof(avg_hashrate_buf), " avg " CYAN_BOLD("%s %s"), Hashrate::format({ true, avg_hashrate * scale }, num + 16 * 4, 16), h);
        }
#       endif

        LOG_INFO("%s " WHITE_BOLD("speed") " 10s/60s/15m " CYAN_BOLD("%s") CYAN(" %s %s ") CYAN_BOLD("%s") " max " CYAN_BOLD("%s %s") "%s",
                 Tags::miner(),
                 Hashrate::format(speed[0],                 num,          16),
                 Hashrate::format(speed[1],                 num + 16,     16),
                 Hashrate::format(speed[2],                 num + 16 * 2, 16), h,
                 Hashrate::format({ maxHashrate[algorithm] > 0.0, maxHashrate[algorithm] * scale },   num + 16 * 3, 16), h,
                 avg_hashrate_buf
                 );

        if (count > 1) {
            for (auto backend : backends) {
                const auto hashrate = backend->hashrate();
                if (!hashrate) {
                    continue;
                }

                char backend_num[16 * 3] = { 0 };
                auto backend_short       = hashrate->calc(Hashrate::ShortInterval);
                auto backend_medium      = hashrate->calc(Hashrate::MediumInterval);
                auto backend_large       = hashrate->calc(Hashrate::LargeInterval);
                double backend_scale     = 1.0;
                const char *backend_unit = "H/s";

                if ((backend_short.second >= 1e6) || (backend_medium.second >= 1e6) || (backend_large.second >= 1e6)) {
                    backend_scale = 1e-6;

                    backend_short.second  *= backend_scale;
                    backend_medium.second *= backend_scale;
                    backend_large.second  *= backend_scale;

                    backend_unit = "MH/s";
                }

                LOG_INFO("%s " WHITE_BOLD("%s speed") " 10s/60s/15m " CYAN_BOLD("%s") CYAN(" %s %s ") CYAN_BOLD("%s"),
                         Tags::miner(),
                         backend->type().data(),
                         Hashrate::format(backend_short,  backend_num,          16),
                         Hashrate::format(backend_medium, backend_num + 16,     16),
                         Hashrate::format(backend_large,  backend_num + 16 * 2, 16), backend_unit
                         );
            }
        }

#       ifdef XMRIG_FEATURE_BENCHMARK
        for (auto backend : backends) {
            backend->printBenchProgress();
        }
#       endif
    }


#   ifdef XMRIG_ALGO_RANDOMX
    inline bool initRX() const { return Rx::init(job, controller->config()->rx(), controller->config()->cpu()); }
#   endif


#   ifdef XMRIG_ALGO_GHOSTRIDER
    inline void initGhostRider() const { ghostrider::benchmark(); }
#   endif


    Algorithm algorithm;
    Algorithms algorithms;
    bool active         = false;
    bool battery_power  = false;
    bool user_active    = false;
    bool enabled        = true;
    int32_t auto_pause = 0;
    bool reset          = true;
    bool backendSplitLogged = false;
    Controller *controller;
    Job job;
    std::array<Job, 2> poolJobs;
    int activePool      = -1;
    int64_t minorityCoreAffinity = -1;
    bool poolSwitched   = false;
    mutable std::map<Algorithm::Id, double> maxHashrate;
    std::vector<IBackend *> backends;
    String userJobId;
    Timer *timer        = nullptr;
    uint64_t ticks      = 0;

    Taskbar m_taskbar;
};


} // namespace xmrig



xmrig::Miner::Miner(Controller *controller)
    : d_ptr(new MinerPrivate(controller))
{
    const int priority = controller->config()->cpu().priority();
    if (priority >= 0) {
        Platform::setProcessPriority(priority);
        Platform::setThreadPriority(std::min(priority + 1, 5));
    }

#   ifdef XMRIG_FEATURE_PROFILING
    ProfileScopeData::Init();
#   endif

#   ifdef XMRIG_ALGO_RANDOMX
    Rx::init(this);
#   endif

    controller->addListener(this);

#   ifdef XMRIG_FEATURE_API
    controller->api()->addListener(this);
#   endif

    d_ptr->timer = new Timer(this);

    d_ptr->backends.reserve(3);
    d_ptr->backends.push_back(new CpuBackend(controller));

#   ifdef XMRIG_FEATURE_OPENCL
    d_ptr->backends.push_back(new OclBackend(controller));
#   endif

#   ifdef XMRIG_FEATURE_CUDA
    d_ptr->backends.push_back(new CudaBackend(controller));
#   endif

    d_ptr->rebuild();
}


xmrig::Miner::~Miner()
{
    delete d_ptr;
}


bool xmrig::Miner::isEnabled() const
{
    return d_ptr->enabled;
}


bool xmrig::Miner::isEnabled(const Algorithm &algorithm) const
{
    return std::find(d_ptr->algorithms.begin(), d_ptr->algorithms.end(), algorithm) != d_ptr->algorithms.end();
}


const xmrig::Algorithms &xmrig::Miner::algorithms() const
{
    return d_ptr->algorithms;
}


const std::vector<xmrig::IBackend *> &xmrig::Miner::backends() const
{
    return d_ptr->backends;
}


xmrig::Job xmrig::Miner::job() const
{
    std::lock_guard<std::mutex> lock(mutex);

    return d_ptr->job;
}


bool xmrig::Miner::isMajorityPool(uint8_t poolId) const
{
    std::lock_guard<std::mutex> lock(mutex);

    if (d_ptr->controller->config()->isFixedDualPoolSplit()) {
        return false;
    }

    return !d_ptr->isDualGpuSplit() && d_ptr->isDualPool() && d_ptr->activePool >= 0 && poolId == static_cast<uint8_t>(d_ptr->activePool);
}


bool xmrig::Miner::isDualCpuSplit() const
{
    std::lock_guard<std::mutex> lock(mutex);

    return d_ptr->isDualCpuSplit();
}


int xmrig::Miner::activePool() const
{
    std::lock_guard<std::mutex> lock(mutex);

    return d_ptr->activePool;
}


xmrig::Job xmrig::Miner::job(bool gpu) const
{
    std::lock_guard<std::mutex> lock(mutex);

    return d_ptr->jobForBackend(gpu ? 1 : 0);
}


xmrig::Job xmrig::Miner::job(size_t workerId, int64_t affinity) const
{
    std::lock_guard<std::mutex> lock(mutex);

    return d_ptr->jobForWorker(workerId, affinity);
}


xmrig::Job xmrig::Miner::job(size_t workerId, int64_t affinity, int8_t poolId) const
{
    std::lock_guard<std::mutex> lock(mutex);

    return d_ptr->jobForWorker(workerId, affinity, poolId);
}


xmrig::Job xmrig::Miner::poolJob(uint8_t poolId) const
{
    std::lock_guard<std::mutex> lock(mutex);

    if (poolId < d_ptr->poolJobs.size() && d_ptr->poolJobs[poolId].isValid()) {
        return d_ptr->poolJobs[poolId];
    }

    return d_ptr->job;
}


void xmrig::Miner::execCommand(char command)
{
    switch (command) {
    case 'h':
    case 'H':
        d_ptr->printHashrate(true);
        break;

    case 'p':
    case 'P':
        setEnabled(false);
        break;

    case 'r':
    case 'R':
        setEnabled(true);
        break;

    case 'e':
    case 'E':
        for (auto backend : d_ptr->backends) {
            backend->printHealth();
        }
        break;

    default:
        break;
    }

    for (auto backend : d_ptr->backends) {
        backend->execCommand(command);
    }
}


void xmrig::Miner::pause()
{
    d_ptr->active = false;
    d_ptr->m_taskbar.setActive(false);

    Nonce::pause(true);
    Nonce::touch();
}


void xmrig::Miner::setEnabled(bool enabled)
{
    if (d_ptr->enabled == enabled) {
        return;
    }

    if (d_ptr->controller->config()->isPauseOnBattery() && d_ptr->battery_power && enabled) {
        LOG_INFO("%s " YELLOW_BOLD("can't resume while on battery power"), Tags::miner());

        return;
    }

    d_ptr->enabled = enabled;
    d_ptr->m_taskbar.setEnabled(enabled);

    if (enabled) {
        LOG_INFO("%s " GREEN_BOLD("resumed"), Tags::miner());
    }
    else {
        if (d_ptr->battery_power) {
            LOG_INFO("%s " YELLOW_BOLD("paused"), Tags::miner());
        }
        else {
            LOG_INFO("%s " YELLOW_BOLD("paused") ", press " MAGENTA_BG_BOLD(" r ") " to resume", Tags::miner());
        }
    }

    if (!d_ptr->active) {
        return;
    }

    Nonce::pause(!enabled);
    Nonce::touch();
}


void xmrig::Miner::setJob(const Job &job, bool donate)
{
    Job nextJob = job;
    bool dualPoolJob = !donate && d_ptr->isDualPool() && nextJob.poolId() < d_ptr->poolJobs.size();
    const uint8_t index = donate ? 1 : (dualPoolJob && nextJob.poolId() == 1 ? 2 : 0);
    bool dualCpuSplitJob = false;
    nextJob.setIndex(index);

    if (dualPoolJob) {
        std::lock_guard<std::mutex> lock(mutex);
        const uint8_t otherPoolId = static_cast<uint8_t>(1 - nextJob.poolId());
        if (!d_ptr->isDualGpuSplit() && d_ptr->poolJobs[otherPoolId].isValid() && !MinerPrivate::isDualAlgorithmCompatible(nextJob.algorithm(), d_ptr->poolJobs[otherPoolId].algorithm())) {
            dualCpuSplitJob = true;

            if (!d_ptr->backendSplitLogged) {
                d_ptr->backendSplitLogged = true;
                if (d_ptr->controller->config()->isFixedDualPoolSplit()) {
                    LOG_INFO("%s dual-pool CPU fixed split enabled: pool 0 %u%% -> %s, pool 1 %u%% -> %s",
                             Tags::miner(),
                             d_ptr->controller->config()->splitPool0(),
                             d_ptr->poolJobs[0].isValid() ? d_ptr->poolJobs[0].algorithm().name() : nextJob.algorithm().name(),
                             d_ptr->controller->config()->splitPool1(),
                             d_ptr->poolJobs[1].isValid() ? d_ptr->poolJobs[1].algorithm().name() : nextJob.algorithm().name());
                }
                else {
                    LOG_INFO("%s dual-pool CPU split enabled: pool 0 -> %s, pool 1 -> %s",
                             Tags::miner(), d_ptr->poolJobs[0].isValid() ? d_ptr->poolJobs[0].algorithm().name() : nextJob.algorithm().name(), d_ptr->poolJobs[1].isValid() ? d_ptr->poolJobs[1].algorithm().name() : nextJob.algorithm().name());
                }
            }
        }
    }

#   ifdef XMRIG_ALGO_RANDOMX
    if (nextJob.algorithm().family() == Algorithm::RANDOM_X) {
        if (d_ptr->algorithm != nextJob.algorithm()) {
            const bool sameConfig = d_ptr->algorithm.family() == Algorithm::RANDOM_X && RxAlgo::base(d_ptr->algorithm) == RxAlgo::base(nextJob.algorithm());
            if (!sameConfig && !dualCpuSplitJob) {
                stop();
            }

            RxAlgo::apply(nextJob.algorithm());
        }
        else if (!Rx::isReady(nextJob)) {
            Nonce::pause(true);
            Nonce::touch();
        }
    }
#   endif

    mutex.lock();

    const bool same_job_index = d_ptr->job.index() == index;
    bool resetUpdatedPoolJob = false;

    if (dualPoolJob) {
        const Job previousPoolJob = d_ptr->poolJobs[nextJob.poolId()];
        resetUpdatedPoolJob = previousPoolJob.isValid() && !previousPoolJob.isEqualBlob(nextJob);
        d_ptr->poolJobs[nextJob.poolId()] = nextJob;

        if (d_ptr->poolJobs[0].isValid()) {
            if (!d_ptr->poolSwitched || d_ptr->activePool < 0 || !d_ptr->poolJobs[static_cast<size_t>(d_ptr->activePool)].isValid()) {
                d_ptr->activePool = 0;
            }
        }
        else if (d_ptr->activePool < 0) {
            d_ptr->activePool = nextJob.poolId();
        }

        if (d_ptr->isDualGpuSplit() && d_ptr->poolJobs[0].isValid() && d_ptr->poolJobs[1].isValid() && !d_ptr->backendSplitLogged) {
            d_ptr->backendSplitLogged = true;
            LOG_INFO("%s dual-pool backend split enabled: CPU -> pool 0, GPU -> pool 1", Tags::miner());
        }
    }

    const Job previousJob = d_ptr->job;
    const Job selectedJob = dualPoolJob ? d_ptr->poolJobs[static_cast<size_t>(d_ptr->activePool)] : nextJob;

    d_ptr->reset = !(previousJob.index() == 1 && index == 0 && d_ptr->userJobId == selectedJob.id());

    // Don't reset nonce if pool sends the same hashing blob again, but with different difficulty (for example)
    if (previousJob.isEqualBlob(selectedJob)) {
        d_ptr->reset = false;
    }

    d_ptr->job = selectedJob;
    d_ptr->algorithm = selectedJob.algorithm();

    if (!donate) {
        d_ptr->userJobId = selectedJob.id();
    }

#   ifdef XMRIG_ALGO_RANDOMX
    const bool nextReady = nextJob.algorithm().family() == Algorithm::RANDOM_X ? Rx::init(nextJob, d_ptr->controller->config()->rx(), d_ptr->controller->config()->cpu()) : true;
    const bool selectedReady = selectedJob.algorithm().family() == Algorithm::RANDOM_X ? Rx::init(selectedJob, d_ptr->controller->config()->rx(), d_ptr->controller->config()->cpu()) : true;
    const bool ready = selectedJob.isEqual(nextJob) ? nextReady : (nextReady && selectedReady);

    // Always reset nonce on RandomX dataset change
    // Except for switching to/from donation
    if (!ready && same_job_index) {
        d_ptr->reset = true;
    }
#   else
    constexpr const bool ready = true;
#   endif

#   ifdef XMRIG_ALGO_GHOSTRIDER
    if (job.algorithm().family() == Algorithm::GHOSTRIDER) {
        d_ptr->initGhostRider();
    }
#   endif

    mutex.unlock();

    if (resetUpdatedPoolJob) {
        Nonce::reset(nextJob.index());
    }

    for (size_t i = 0; i < d_ptr->backends.size(); ++i) {
        d_ptr->backends[i]->prepare(this->job(i != 0));
    }

    d_ptr->active = true;
    d_ptr->m_taskbar.setActive(true);

    if (ready) {
        d_ptr->handleJobChange();
    }
}


void xmrig::Miner::stop()
{
    Nonce::stop();

    for (IBackend *backend : d_ptr->backends) {
        backend->stop();
    }
}


void xmrig::Miner::switchPool(uint8_t acceptedPoolId)
{
    if (!d_ptr->isDualPool() || d_ptr->isDualGpuSplit() || d_ptr->controller->config()->isFixedDualPoolSplit()) {
        return;
    }

    mutex.lock();

    if (d_ptr->activePool < 0 || acceptedPoolId != static_cast<uint8_t>(d_ptr->activePool) || !d_ptr->poolJobs[0].isValid() || !d_ptr->poolJobs[1].isValid()) {
        mutex.unlock();

        return;
    }

    const int previousPool = d_ptr->activePool;
    d_ptr->activePool = 1 - d_ptr->activePool;
    d_ptr->poolSwitched = true;
    const int nextPool = d_ptr->activePool;
    const Job previousJob = d_ptr->job;
    d_ptr->job = d_ptr->poolJobs[static_cast<size_t>(d_ptr->activePool)];
    d_ptr->reset = !previousJob.isEqualBlob(d_ptr->job);

#   ifdef XMRIG_ALGO_RANDOMX
    const bool ready = Rx::init(d_ptr->job, d_ptr->controller->config()->rx(), d_ptr->controller->config()->cpu());
#   else
    constexpr const bool ready = true;
#   endif

    mutex.unlock();

    LOG_INFO("%s dual-pool majority switched pool %d -> pool %d (one physical core is minority)", Tags::miner(), previousPool, nextPool);

    if (ready) {
        d_ptr->handleJobChange();
    }
}


void xmrig::Miner::onConfigChanged(Config *config, Config *previousConfig)
{
    d_ptr->rebuild();

    if (config->pools() != previousConfig->pools() && config->pools().active() > 0) {
        return;
    }

    const Job job = this->job();

    for (IBackend *backend : d_ptr->backends) {
        backend->setJob(job);
    }
}


void xmrig::Miner::onTimer(const Timer *)
{
    double maxHashrate          = 0.0;
    const auto config           = d_ptr->controller->config();
    const auto healthPrintTime  = config->healthPrintTime();

    bool stopMiner = false;

    for (IBackend *backend : d_ptr->backends) {
        if (!backend->tick(d_ptr->ticks)) {
            stopMiner = true;
        }

        if (healthPrintTime && d_ptr->ticks && (d_ptr->ticks % (healthPrintTime * 2)) == 0 && backend->isEnabled()) {
            backend->printHealth();
        }

        if (backend->hashrate()) {
            const auto h = backend->hashrate()->calc(Hashrate::ShortInterval);
            if (h.first) {
                maxHashrate += h.second;
            }
        }
    }

    d_ptr->maxHashrate[d_ptr->algorithm] = std::max(d_ptr->maxHashrate[d_ptr->algorithm], maxHashrate);

    const auto printTime = config->printTime();
    if (printTime && d_ptr->ticks && (d_ptr->ticks % (printTime * 2)) == 0) {
        d_ptr->printHashrate(false);
    }

    d_ptr->ticks++;

    auto autoPause = [this](bool &state, bool pause, const char *pauseMessage, const char *activeMessage)
    {
        if ((pause && !state) || (!pause && state)) {
            LOG_INFO("%s %s", Tags::miner(), pause ? pauseMessage : activeMessage);

            state = pause;
            d_ptr->auto_pause += pause ? 1 : -1;
            setEnabled(d_ptr->auto_pause == 0);
        }
    };

    if (config->isPauseOnBattery()) {
        autoPause(d_ptr->battery_power, Platform::isOnBatteryPower(), YELLOW_BOLD("on battery power"), GREEN_BOLD("on AC power"));
    }

    if (config->isPauseOnActive()) {
        autoPause(d_ptr->user_active, Platform::isUserActive(config->idleTime()), YELLOW_BOLD("user active"), GREEN_BOLD("user inactive"));
    }

    if (stopMiner) {
        stop();
    }
}


#ifdef XMRIG_FEATURE_API
void xmrig::Miner::onRequest(IApiRequest &request)
{
    if (request.method() == IApiRequest::METHOD_GET) {
        if (request.type() == IApiRequest::REQ_SUMMARY) {
            request.accept();

            d_ptr->getMiner(request.reply(), request.doc(), request.version());
            d_ptr->getHashrate(request.reply(), request.doc(), request.version());
        }
        else if (request.url() == "/2/backends") {
            request.accept();

            d_ptr->getBackends(request.reply(), request.doc());
        }
    }
    else if (request.type() == IApiRequest::REQ_JSON_RPC) {
        if (request.rpcMethod() == "pause") {
            request.accept();

            setEnabled(false);
        }
        else if (request.rpcMethod() == "resume") {
            request.accept();

            setEnabled(true);
        }
        else if (request.rpcMethod() == "stop") {
            request.accept();

            stop();
        }
    }

    for (IBackend *backend : d_ptr->backends) {
        backend->handleRequest(request);
    }
}
#endif


#ifdef XMRIG_ALGO_RANDOMX
void xmrig::Miner::onDatasetReady()
{
    if (!Rx::isReady(job())) {
        return;
    }

    d_ptr->handleJobChange();
}
#endif
