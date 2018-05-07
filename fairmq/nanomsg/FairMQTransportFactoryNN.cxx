/********************************************************************************
 * Copyright (C) 2014-2017 GSI Helmholtzzentrum fuer Schwerionenforschung Gmb   *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "FairMQTransportFactoryNN.h"

#include <nanomsg/nn.h>

using namespace std;

fair::mq::Transport FairMQTransportFactoryNN::fTransportType = fair::mq::Transport::NN;

FairMQTransportFactoryNN::FairMQTransportFactoryNN(const string& id, const FairMQProgOptions* /*config*/)
    : FairMQTransportFactory(id)
{
    LOG(debug) << "Transport: Using nanomsg library";
}

FairMQMessagePtr FairMQTransportFactoryNN::CreateMessage() const
{
    return unique_ptr<FairMQMessage>(new FairMQMessageNN());
}

FairMQMessagePtr FairMQTransportFactoryNN::CreateMessage(const size_t size) const
{
    return unique_ptr<FairMQMessage>(new FairMQMessageNN(size));
}

FairMQMessagePtr FairMQTransportFactoryNN::CreateMessage(void* data, const size_t size, fairmq_free_fn* ffn, void* hint) const
{
    return unique_ptr<FairMQMessage>(new FairMQMessageNN(data, size, ffn, hint));
}

FairMQMessagePtr FairMQTransportFactoryNN::CreateMessage(FairMQUnmanagedRegionPtr& region, void* data, const size_t size, void* hint) const
{
    return unique_ptr<FairMQMessage>(new FairMQMessageNN(region, data, size, hint));
}

FairMQSocketPtr FairMQTransportFactoryNN::CreateSocket(const string& type, const string& name) const
{
    return unique_ptr<FairMQSocket>(new FairMQSocketNN(type, name, GetId()));
}

FairMQPollerPtr FairMQTransportFactoryNN::CreatePoller(const vector<FairMQChannel>& channels) const
{
    return unique_ptr<FairMQPoller>(new FairMQPollerNN(channels));
}

FairMQPollerPtr FairMQTransportFactoryNN::CreatePoller(const std::vector<const FairMQChannel*>& channels) const
{
    return unique_ptr<FairMQPoller>(new FairMQPollerNN(channels));
}

FairMQPollerPtr FairMQTransportFactoryNN::CreatePoller(const unordered_map<string, vector<FairMQChannel>>& channelsMap, const vector<string>& channelList) const
{
    return unique_ptr<FairMQPoller>(new FairMQPollerNN(channelsMap, channelList));
}

FairMQPollerPtr FairMQTransportFactoryNN::CreatePoller(const FairMQSocket& cmdSocket, const FairMQSocket& dataSocket) const
{
    return unique_ptr<FairMQPoller>(new FairMQPollerNN(cmdSocket, dataSocket));
}

FairMQUnmanagedRegionPtr FairMQTransportFactoryNN::CreateUnmanagedRegion(const size_t size, FairMQRegionCallback callback) const
{
    return unique_ptr<FairMQUnmanagedRegion>(new FairMQUnmanagedRegionNN(size, callback));
}

fair::mq::Transport FairMQTransportFactoryNN::GetType() const
{
    return fTransportType;
}

FairMQTransportFactoryNN::~FairMQTransportFactoryNN()
{
    // nn_term();
    // see https://www.freelists.org/post/nanomsg/Getting-rid-of-nn-init-and-nn-term,8
}
