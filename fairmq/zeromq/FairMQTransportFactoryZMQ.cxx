/********************************************************************************
 * Copyright (C) 2014-2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "FairMQTransportFactoryZMQ.h"
#include <fairmq/Tools.h>
#include <zmq.h>

#include <algorithm> // find_if

using namespace std;

FairMQTransportFactoryZMQ::FairMQTransportFactoryZMQ(const string& id, const fair::mq::ProgOptions* config)
    : FairMQTransportFactory(id)
    , fCtx(nullptr)
{
    int major, minor, patch;
    zmq_version(&major, &minor, &patch);
    LOG(debug) << "Transport: Using ZeroMQ library, version: " << major << "." << minor << "." << patch;

    if (config) {
        fCtx = fair::mq::tools::make_unique<fair::mq::zmq::Context>(config->GetProperty<int>("io-threads", 1));
    } else {
        LOG(debug) << "fair::mq::ProgOptions not available! Using defaults.";
        fCtx = fair::mq::tools::make_unique<fair::mq::zmq::Context>(1);
    }
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
    return unique_ptr<FairMQSocket>(new FairMQSocketZMQ(*fCtx, type, name, GetId(), this));
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

FairMQUnmanagedRegionPtr FairMQTransportFactoryZMQ::CreateUnmanagedRegion(
    const size_t size,
    FairMQRegionCallback callback,
    const string& path /* = "" */,
    int flags /* = 0 */)
{
    return CreateUnmanagedRegion(size, 0, callback, nullptr, path, flags);
}
FairMQUnmanagedRegionPtr FairMQTransportFactoryZMQ::CreateUnmanagedRegion(
    const size_t size,
    FairMQRegionBulkCallback bulkCallback,
    const string& path /* = "" */,
    int flags /* = 0 */)
{
    return CreateUnmanagedRegion(size, 0, nullptr, bulkCallback, path, flags);
}
FairMQUnmanagedRegionPtr FairMQTransportFactoryZMQ::CreateUnmanagedRegion(
    const size_t size,
    const int64_t userFlags,
    FairMQRegionCallback callback,
    const string& path /* = "" */,
    int flags /* = 0 */)
{
    return CreateUnmanagedRegion(size, userFlags, callback, nullptr, path, flags);
}
FairMQUnmanagedRegionPtr FairMQTransportFactoryZMQ::CreateUnmanagedRegion(
    const size_t size,
    const int64_t userFlags,
    FairMQRegionBulkCallback bulkCallback,
    const string& path /* = "" */,
    int flags /* = 0 */)
{
    return CreateUnmanagedRegion(size, userFlags, nullptr, bulkCallback, path, flags);
}

FairMQUnmanagedRegionPtr FairMQTransportFactoryZMQ::CreateUnmanagedRegion(
    const size_t size,
    const int64_t userFlags,
    FairMQRegionCallback callback,
    FairMQRegionBulkCallback bulkCallback,
    const string& path /* = "" */,
    int flags /* = 0 */)
{
    unique_ptr<FairMQUnmanagedRegion> ptr = unique_ptr<FairMQUnmanagedRegion>(new FairMQUnmanagedRegionZMQ(*fCtx, size, userFlags, callback, bulkCallback, path, flags, this));
    auto zPtr = static_cast<FairMQUnmanagedRegionZMQ*>(ptr.get());
    fCtx->AddRegion(zPtr->GetId(), zPtr->GetData(), zPtr->GetSize(), zPtr->GetUserFlags(), fair::mq::RegionEvent::created);
    return ptr;
}

FairMQTransportFactoryZMQ::~FairMQTransportFactoryZMQ()
{
    LOG(debug) << "Destroying ZeroMQ transport...";
}
