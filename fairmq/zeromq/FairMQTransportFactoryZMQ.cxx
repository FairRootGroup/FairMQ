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

FairMQ::Transport FairMQTransportFactoryZMQ::fTransportType = FairMQ::Transport::ZMQ;

FairMQTransportFactoryZMQ::FairMQTransportFactoryZMQ(const string& id, const FairMQProgOptions* config)
    : FairMQTransportFactory(id)
    , fContext(zmq_ctx_new())
{
    int major, minor, patch;
    zmq_version(&major, &minor, &patch);
    LOG(DEBUG) << "Transport: Using ZeroMQ library, version: " << major << "." << minor << "." << patch;

    if (!fContext)
    {
        LOG(ERROR) << "failed creating context, reason: " << zmq_strerror(errno);
        exit(EXIT_FAILURE);
    }

    if (zmq_ctx_set(fContext, ZMQ_MAX_SOCKETS, 10000) != 0)
    {
        LOG(ERROR) << "failed configuring context, reason: " << zmq_strerror(errno);
    }

    int numIoThreads = 1;
    if (config)
    {
        numIoThreads = config->GetValue<int>("io-threads");
    }
    else
    {
        LOG(WARN) << "zeromq: FairMQProgOptions not available! Using defaults.";
    }

    if (zmq_ctx_set(fContext, ZMQ_IO_THREADS, numIoThreads) != 0)
    {
        LOG(ERROR) << "failed configuring context, reason: " << zmq_strerror(errno);
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

FairMQMessagePtr FairMQTransportFactoryZMQ::CreateMessage(FairMQRegionPtr& region, void* data, const size_t size) const
{
    return unique_ptr<FairMQMessage>(new FairMQMessageZMQ(region, data, size));
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

FairMQPollerPtr FairMQTransportFactoryZMQ::CreatePoller(const FairMQSocket& cmdSocket, const FairMQSocket& dataSocket) const
{
    return unique_ptr<FairMQPoller>(new FairMQPollerZMQ(cmdSocket, dataSocket));
}

FairMQRegionPtr FairMQTransportFactoryZMQ::CreateRegion(const size_t size) const
{
    return unique_ptr<FairMQRegion>(new FairMQRegionZMQ(size));
}

FairMQ::Transport FairMQTransportFactoryZMQ::GetType() const
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
                LOG(ERROR) << " failed closing context, reason: " << zmq_strerror(errno);
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
        LOG(ERROR) << "Terminate(): context not available for shutdown";
    }
}
