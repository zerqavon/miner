/* XMRig
 * Copyright (c) 2018      Lee Clagett <https://github.com/vtnerd>
 * Copyright (c) 2018-2019 tevador     <tevador@gmail.com>
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

#ifndef XMRIG_RX_QUEUE_H
#define XMRIG_RX_QUEUE_H


#include "base/kernel/interfaces/IAsyncListener.h"
#include "base/tools/Object.h"
#include "crypto/common/HugePagesInfo.h"
#include "crypto/rx/RxConfig.h"
#include "crypto/rx/RxSeed.h"


#include <condition_variable>
#include <mutex>
#include <utility>
#include <thread>


namespace xmrig
{


class IRxListener;
class IRxStorage;
class RxDataset;


class RxQueueItem
{
public:
    RxQueueItem(const RxSeed &seed, const std::vector<uint32_t> &nodeset, uint32_t threads, bool hugePages, bool oneGbPages, RxConfig::Mode mode, int priority, IRxStorage *storage) :
        hugePages(hugePages),
        oneGbPages(oneGbPages),
        priority(priority),
        mode(mode),
        seed(seed),
        nodeset(nodeset),
        threads(threads),
        storage(storage)
    {}

    bool hugePages;
    bool oneGbPages;
    int priority;
    RxConfig::Mode mode;
    RxSeed seed;
    std::vector<uint32_t> nodeset;
    uint32_t threads;
    IRxStorage *storage;
};


class RxStorageItem
{
public:
    RxStorageItem(const RxSeed &seed, IRxStorage *storage) :
        seed(seed),
        storage(storage)
    {}

    RxSeed seed;
    IRxStorage *storage;
    bool ready = false;
};


class RxQueue : public IAsyncListener
{
public:
    XMRIG_DISABLE_COPY_MOVE(RxQueue);

    RxQueue(IRxListener *listener);
    ~RxQueue() override;

    HugePagesInfo hugePages();
    RxDataset *dataset(const Job &job, uint32_t nodeId);
    template<typename T> bool isReady(const T &seed);
    void enqueue(const RxSeed &seed, const std::vector<uint32_t> &nodeset, uint32_t threads, bool hugePages, bool oneGbPages, RxConfig::Mode mode, int priority);

protected:
    inline void onAsync() override  { onReady(); }

private:
    enum State {
        STATE_IDLE,
        STATE_PENDING,
        STATE_SHUTDOWN
    };

    template<typename T> bool isReadyUnsafe(const T &seed) const;
    IRxStorage *createStorage(const std::vector<uint32_t> &nodeset) const;
    RxStorageItem *findStorageUnsafe(const RxSeed &seed);
    const RxStorageItem *findStorageUnsafe(const RxSeed &seed) const;
    void backgroundInit();
    void onReady();

    IRxListener *m_listener = nullptr;
    State m_state = STATE_IDLE;
    std::condition_variable m_cv;
    std::mutex m_mutex;
    std::shared_ptr<Async> m_async;
    std::thread m_thread;
    std::vector<RxStorageItem> m_storages;
    std::vector<RxQueueItem> m_queue;
};


} /* namespace xmrig */


#endif /* XMRIG_RX_QUEUE_H */
