/********************************************************************************
 * Copyright (C) 2014-2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "FairMQTransportFactoryZMQ.h"
#include <zmq.h>

#include <algorithm> // find_if

using namespace std;

fair::mq::Transport FairMQTransportFactoryZMQ::fTransportType = fair::mq::Transport::ZMQ;

FairMQTransportFactoryZMQ::FairMQTransportFactoryZMQ(const string& id, const fair::mq::ProgOptions* config)
    : FairMQTransportFactory(id)
    , fContext(zmq_ctx_new())
    , fRegionCounter(0)
{
    int major, minor, patch;
    zmq_version(&major, &minor, &patch);
    LOG(debug) << "Transport: Using ZeroMQ library, version: " << major << "." << minor << "." << patch;

    if (!fContext)
    {
        LOG(error) << "failed creating context, reason: " << zmq_strerror(errno);
        exit(EXIT_FAILURE);
    }

    if (zmq_ctx_set(fContext, ZMQ_MAX_SOCKETS, 10000) != 0)
    {
        LOG(error) << "failed configuring context, reason: " << zmq_strerror(errno);
    }

    int numIoThreads = 1;
    if (config)
    {
        numIoThreads = config->GetProperty<int>("io-threads", numIoThreads);
    }
    else
    {
        LOG(debug) << "fair::mq::ProgOptions not available! Using defaults.";
    }

    if (zmq_ctx_set(fContext, ZMQ_IO_THREADS, numIoThreads) != 0)
    {
        LOG(error) << "failed configuring context, reason: " << zmq_strerror(errno);
    }

    fRegionEvents.emplace(0, nullptr, 0, 0, fair::mq::RegionEvent::local_only);
}

FairMQMessagePtr FairMQTransportFactoryZMQ::CreateMessage()
{
    return unique_ptr<FairMQMessage>(new FairMQMessageZMQ(this));
}

FairMQMessagePtr FairMQTransportFactoryZMQ::CreateMessage(const size_t size)
{
    return unique_ptr<FairMQMessage>(new FairMQMessageZMQ(size, this));
}

FairMQMessagePtr FairMQTransportFactoryZMQ::CreateMessage(void* data, const size_t size, fairmq_free_fn* ffn, void* hint)
{
    return unique_ptr<FairMQMessage>(new FairMQMessageZMQ(data, size, ffn, hint, this));
}

FairMQMessagePtr FairMQTransportFactoryZMQ::CreateMessage(FairMQUnmanagedRegionPtr& region, void* data, const size_t size, void* hint)
{
    return unique_ptr<FairMQMessage>(new FairMQMessageZMQ(region, data, size, hint, this));
}

FairMQSocketPtr FairMQTransportFactoryZMQ::CreateSocket(const string& type, const string& name)
{
    assert(fContext);
    return unique_ptr<FairMQSocket>(new FairMQSocketZMQ(type, name, GetId(), fContext, this));
}

FairMQPollerPtr FairMQTransportFactoryZMQ::CreatePoller(const vector<FairMQChannel>& channels) const
{
    return unique_ptr<FairMQPoller>(new FairMQPollerZMQ(channels));
}

FairMQPollerPtr FairMQTransportFactoryZMQ::CreatePoller(const vector<FairMQChannel*>& channels) const
{
    return unique_ptr<FairMQPoller>(new FairMQPollerZMQ(channels));
}

FairMQPollerPtr FairMQTransportFactoryZMQ::CreatePoller(const unordered_map<string, vector<FairMQChannel>>& channelsMap, const vector<string>& channelList) const
{
    return unique_ptr<FairMQPoller>(new FairMQPollerZMQ(channelsMap, channelList));
}

FairMQUnmanagedRegionPtr FairMQTransportFactoryZMQ::CreateUnmanagedRegion(const size_t size, FairMQRegionCallback callback, const string& path /* = "" */, int flags /* = 0 */)
{
    return CreateUnmanagedRegion(size, 0, callback, path, flags);
}

FairMQUnmanagedRegionPtr FairMQTransportFactoryZMQ::CreateUnmanagedRegion(const size_t size, const int64_t userFlags, FairMQRegionCallback callback, const string& path /* = "" */, int flags /* = 0 */)
{
    unique_ptr<FairMQUnmanagedRegion> ptr = nullptr;
    {
        lock_guard<mutex> lock(fMtx);

        ++fRegionCounter;
        ptr = unique_ptr<FairMQUnmanagedRegion>(new FairMQUnmanagedRegionZMQ(fRegionCounter, size, userFlags, callback, path, flags, this));
        auto zPtr = static_cast<FairMQUnmanagedRegionZMQ*>(ptr.get());
        fRegionInfos.emplace_back(zPtr->GetId(), zPtr->GetData(), zPtr->GetSize(), zPtr->GetUserFlags(), fair::mq::RegionEvent::created);
        fRegionEvents.emplace(zPtr->GetId(), zPtr->GetData(), zPtr->GetSize(), zPtr->GetUserFlags(), fair::mq::RegionEvent::created);
    }
    fRegionEventsCV.notify_one();
    return ptr;
}

void FairMQTransportFactoryZMQ::SubscribeToRegionEvents(FairMQRegionEventCallback callback)
{
    if (fRegionEventThread.joinable()) {
        LOG(debug) << "Already subscribed. Overwriting previous subscription.";
        {
            lock_guard<mutex> lock(fMtx);
            fRegionEventsSubscriptionActive = false;
        }
        fRegionEventsCV.notify_one();
        fRegionEventThread.join();
    }
    lock_guard<mutex> lock(fMtx);
    fRegionEventCallback = callback;
    fRegionEventsSubscriptionActive = true;
    fRegionEventThread = thread(&FairMQTransportFactoryZMQ::RegionEventsSubscription, this);
}

void FairMQTransportFactoryZMQ::UnsubscribeFromRegionEvents()
{
    if (fRegionEventThread.joinable()) {
        unique_lock<mutex> lock(fMtx);
        fRegionEventsSubscriptionActive = false;
        lock.unlock();
        fRegionEventsCV.notify_one();
        fRegionEventThread.join();
        lock.lock();
        fRegionEventCallback = nullptr;
    }
}

void FairMQTransportFactoryZMQ::RegionEventsSubscription()
{
    unique_lock<mutex> lock(fMtx);
    while (fRegionEventsSubscriptionActive) {

        while (!fRegionEvents.empty()) {
            auto i = fRegionEvents.front();
            fRegionEventCallback(i);
            fRegionEvents.pop();
        }
        fRegionEventsCV.wait(lock, [&]() { return !fRegionEventsSubscriptionActive || !fRegionEvents.empty(); });
    }
}

vector<fair::mq::RegionInfo> FairMQTransportFactoryZMQ::GetRegionInfo()
{
    lock_guard<mutex> lock(fMtx);
    return fRegionInfos;
}

void FairMQTransportFactoryZMQ::RemoveRegion(uint64_t id)
{
    {
        lock_guard<mutex> lock(fMtx);
        auto it = find_if(fRegionInfos.begin(), fRegionInfos.end(), [id](const fair::mq::RegionInfo& i) {
            return i.id == id;
        });
        if (it != fRegionInfos.end()) {
            fRegionEvents.push(*it);
            fRegionEvents.back().event = fair::mq::RegionEvent::destroyed;
            fRegionInfos.erase(it);
        } else {
            LOG(error) << "RemoveRegion: given id (" << id << ") not found.";
        }
    }
    fRegionEventsCV.notify_one();
}

fair::mq::Transport FairMQTransportFactoryZMQ::GetType() const
{
    return fTransportType;
}

FairMQTransportFactoryZMQ::~FairMQTransportFactoryZMQ()
{
    LOG(debug) << "Destroying ZeroMQ transport...";

    UnsubscribeFromRegionEvents();

    if (fContext) {
        if (zmq_ctx_term(fContext) != 0) {
            if (errno == EINTR) {
                LOG(error) << " failed closing context, reason: " << zmq_strerror(errno);
            } else {
                fContext = nullptr;
                return;
            }
        }
    } else {
        LOG(error) << "context not available for shutdown";
    }
}
