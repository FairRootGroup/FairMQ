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

FairMQTransportFactoryZMQ::FairMQTransportFactoryZMQ(const string& id, const FairMQProgOptions* config)
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
        numIoThreads = config->GetValue<int>("io-threads");
    }
    else
    {
        LOG(debug) << "FairMQProgOptions not available! Using defaults.";
    }

    if (zmq_ctx_set(fContext, ZMQ_IO_THREADS, numIoThreads) != 0)
    {
        LOG(error) << "failed configuring context, reason: " << zmq_strerror(errno);
    }

}

FairMQMessagePtr FairMQTransportFactoryZMQ::CreateMessage() const
{
    return unique_ptr<FairMQMessage>(new FairMQMessageZMQ());
}

FairMQMessagePtr FairMQTransportFactoryZMQ::CreateMessage(const size_t size) const
{
    return unique_ptr<FairMQMessage>(new FairMQMessageZMQ(size));
}

FairMQMessagePtr FairMQTransportFactoryZMQ::CreateMessage(void* data, const size_t size, fairmq_free_fn* ffn, void* hint) const
{
    return unique_ptr<FairMQMessage>(new FairMQMessageZMQ(data, size, ffn, hint));
}

FairMQMessagePtr FairMQTransportFactoryZMQ::CreateMessage(FairMQUnmanagedRegionPtr& region, void* data, const size_t size, void* hint) const
{
    return unique_ptr<FairMQMessage>(new FairMQMessageZMQ(region, data, size, hint));
}

FairMQSocketPtr FairMQTransportFactoryZMQ::CreateSocket(const string& type, const string& name) const
{
    assert(fContext);
    return unique_ptr<FairMQSocket>(new FairMQSocketZMQ(type, name, GetId(), fContext));
}

FairMQPollerPtr FairMQTransportFactoryZMQ::CreatePoller(const vector<FairMQChannel>& channels) const
{
    return unique_ptr<FairMQPoller>(new FairMQPollerZMQ(channels));
}

FairMQPollerPtr FairMQTransportFactoryZMQ::CreatePoller(const std::vector<const FairMQChannel*>& channels) const
{
    return unique_ptr<FairMQPoller>(new FairMQPollerZMQ(channels));
}

FairMQPollerPtr FairMQTransportFactoryZMQ::CreatePoller(const unordered_map<string, vector<FairMQChannel>>& channelsMap, const vector<string>& channelList) const
{
    return unique_ptr<FairMQPoller>(new FairMQPollerZMQ(channelsMap, channelList));
}

FairMQUnmanagedRegionPtr FairMQTransportFactoryZMQ::CreateUnmanagedRegion(const size_t size, FairMQRegionCallback callback) const
{
    return unique_ptr<FairMQUnmanagedRegion>(new FairMQUnmanagedRegionZMQ(size, callback));
}

fair::mq::Transport FairMQTransportFactoryZMQ::GetType() const
{
    return fTransportType;
}

FairMQTransportFactoryZMQ::~FairMQTransportFactoryZMQ()
{
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
