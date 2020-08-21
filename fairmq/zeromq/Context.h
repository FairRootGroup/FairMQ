/********************************************************************************
 *    Copyright (C) 2020 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_ZMQ_CONTEXT_H_
#define FAIR_MQ_ZMQ_CONTEXT_H_

#include <fairmq/Tools.h>
#include <FairMQLogger.h>
#include <FairMQUnmanagedRegion.h>

#include <zmq.h>

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace fair
{
namespace mq
{
namespace zmq
{

struct ContextError : std::runtime_error { using std::runtime_error::runtime_error; };

class Context
{
  public:
    Context(int numIoThreads)
        : fZmqCtx(zmq_ctx_new())
        , fInterrupted(false)
        , fRegionCounter(1)
    {
        if (!fZmqCtx) {
            throw ContextError(tools::ToString("failed creating context, reason: ", zmq_strerror(errno)));
        }

        if (zmq_ctx_set(fZmqCtx, ZMQ_MAX_SOCKETS, 10000) != 0) {
            LOG(error) << "failed configuring context, reason: " << zmq_strerror(errno);
            throw ContextError(tools::ToString("failed configuring context, reason: ", zmq_strerror(errno)));
        }

        if (zmq_ctx_set(fZmqCtx, ZMQ_IO_THREADS, numIoThreads) != 0) {
            LOG(error) << "failed configuring context, reason: " << zmq_strerror(errno);
            throw ContextError(tools::ToString("failed configuring context, reason: ", zmq_strerror(errno)));
        }

        fRegionEvents.emplace(true, 0, nullptr, 0, 0, RegionEvent::local_only);
    }

    Context(const Context&) = delete;
    Context operator=(const Context&) = delete;

    void SubscribeToRegionEvents(RegionEventCallback callback)
    {
        if (fRegionEventThread.joinable()) {
            LOG(debug) << "Already subscribed. Overwriting previous subscription.";
            {
                std::lock_guard<std::mutex> lock(fMtx);
                fRegionEventsSubscriptionActive = false;
            }
            fRegionEventsCV.notify_one();
            fRegionEventThread.join();
        }
        std::lock_guard<std::mutex> lock(fMtx);
        fRegionEventCallback = callback;
        fRegionEventsSubscriptionActive = true;
        fRegionEventThread = std::thread(&Context::RegionEventsSubscription, this);
    }

    bool SubscribedToRegionEvents() const { return fRegionEventThread.joinable(); }

    void UnsubscribeFromRegionEvents()
    {
        if (fRegionEventThread.joinable()) {
            std::unique_lock<std::mutex> lock(fMtx);
            fRegionEventsSubscriptionActive = false;
            lock.unlock();
            fRegionEventsCV.notify_one();
            fRegionEventThread.join();
            lock.lock();
            fRegionEventCallback = nullptr;
        }
    }

    void RegionEventsSubscription()
    {
        std::unique_lock<std::mutex> lock(fMtx);
        while (fRegionEventsSubscriptionActive) {

            while (!fRegionEvents.empty()) {
                auto i = fRegionEvents.front();
                fRegionEventCallback(i);
                fRegionEvents.pop();
            }
            fRegionEventsCV.wait(lock, [&]() { return !fRegionEventsSubscriptionActive || !fRegionEvents.empty(); });
        }
    }

    std::vector<RegionInfo> GetRegionInfo() const
    {
        std::lock_guard<std::mutex> lock(fMtx);
        return fRegionInfos;
    }

    uint64_t RegionCount() const
    {
        std::lock_guard<std::mutex> lock(fMtx);
        return fRegionCounter;
    }

    void AddRegion(bool managed, uint64_t id, void* ptr, size_t size, int64_t userFlags, RegionEvent event)
    {
        {
            std::lock_guard<std::mutex> lock(fMtx);
            ++fRegionCounter;
            fRegionInfos.emplace_back(managed, id, ptr, size, userFlags, event);
            fRegionEvents.emplace(managed, id, ptr, size, userFlags, event);
        }
        fRegionEventsCV.notify_one();
    }

    void RemoveRegion(uint64_t id)
    {
        {
            std::lock_guard<std::mutex> lock(fMtx);
            auto it = find_if(fRegionInfos.begin(), fRegionInfos.end(), [id](const RegionInfo& i) {
                return i.id == id;
            });
            if (it != fRegionInfos.end()) {
                fRegionEvents.push(*it);
                fRegionEvents.back().event = RegionEvent::destroyed;
                fRegionInfos.erase(it);
            } else {
                LOG(error) << "RemoveRegion: given id (" << id << ") not found.";
            }
        }
        fRegionEventsCV.notify_one();
    }

    void Interrupt() { fInterrupted.store(true); }
    void Resume() { fInterrupted.store(false); }
    void Reset() {}
    bool Interrupted() { return fInterrupted.load(); }

    void* GetZmqCtx() { return fZmqCtx; }

    ~Context()
    {
        UnsubscribeFromRegionEvents();

        if (fZmqCtx) {
            while (true) {
                if (zmq_ctx_term(fZmqCtx) != 0) {
                    if (errno == EINTR) {
                        LOG(debug) << "zmq_ctx_term interrupted by system call, retrying";
                        continue;
                    } else {
                        fZmqCtx = nullptr;
                    }
                }
                break;
            }
        } else {
            LOG(error) << "context not available for shutdown";
        }
    }

  private:
    void* fZmqCtx;
    mutable std::mutex fMtx;
    std::atomic<bool> fInterrupted;

    uint64_t fRegionCounter;
    std::condition_variable fRegionEventsCV;
    std::vector<RegionInfo> fRegionInfos;
    std::queue<RegionInfo> fRegionEvents;
    std::thread fRegionEventThread;
    std::function<void(RegionInfo)> fRegionEventCallback;
    bool fRegionEventsSubscriptionActive;
};

}   // namespace zmq
}   // namespace mq
}   // namespace fair

#endif /* FAIR_MQ_ZMQ_CONTEXT_H_ */
