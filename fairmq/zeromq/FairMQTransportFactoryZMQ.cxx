/********************************************************************************
 * Copyright (C) 2014-2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "FairMQTransportFactoryZMQ.h"
#include <zmq.h>

using namespace std;

fair::mq::Transport FairMQTransportFactoryZMQ::fTransportType = fair::mq::Transport::ZMQ;

FairMQTransportFactoryZMQ::FairMQTransportFactoryZMQ(const string& id, const fair::mq::ProgOptions* config)
    : FairMQTransportFactory(id)
    , fContext(zmq_ctx_new())
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

FairMQPollerPtr FairMQTransportFactoryZMQ::CreatePoller(const std::vector<FairMQChannel*>& channels) const
{
    return unique_ptr<FairMQPoller>(new FairMQPollerZMQ(channels));
}

FairMQPollerPtr FairMQTransportFactoryZMQ::CreatePoller(const unordered_map<string, vector<FairMQChannel>>& channelsMap, const vector<string>& channelList) const
{
    return unique_ptr<FairMQPoller>(new FairMQPollerZMQ(channelsMap, channelList));
}

FairMQUnmanagedRegionPtr FairMQTransportFactoryZMQ::CreateUnmanagedRegion(const size_t size, FairMQRegionCallback callback, const std::string& path /* = "" */, int flags /* = 0 */) const
{
    return unique_ptr<FairMQUnmanagedRegion>(new FairMQUnmanagedRegionZMQ(size, callback, path, flags));
}

FairMQUnmanagedRegionPtr FairMQTransportFactoryZMQ::CreateUnmanagedRegion(const size_t size, const int64_t userFlags, FairMQRegionCallback callback, const std::string& path /* = "" */, int flags /* = 0 */) const
{
    return unique_ptr<FairMQUnmanagedRegion>(new FairMQUnmanagedRegionZMQ(size, userFlags, callback, path, flags));
}

fair::mq::Transport FairMQTransportFactoryZMQ::GetType() const
{
    return fTransportType;
}

FairMQTransportFactoryZMQ::~FairMQTransportFactoryZMQ()
{
    LOG(debug) << "Destroying ZeroMQ transport...";
    if (fContext)
    {
        if (zmq_ctx_term(fContext) != 0)
        {
            if (errno == EINTR)
            {
                LOG(error) << " failed closing context, reason: " << zmq_strerror(errno);
            }
            else
            {
                fContext = nullptr;
                return;
            }
        }
    }
    else
    {
        LOG(error) << "context not available for shutdown";
    }
}
