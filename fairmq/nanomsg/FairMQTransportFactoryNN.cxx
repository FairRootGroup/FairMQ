/********************************************************************************
 * Copyright (C) 2014-2017 GSI Helmholtzzentrum fuer Schwerionenforschung Gmb   *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "FairMQTransportFactoryNN.h"

#include <nanomsg/nn.h>
#include <algorithm>
#include <thread>
#include <chrono>

using namespace std;

fair::mq::Transport FairMQTransportFactoryNN::fTransportType = fair::mq::Transport::NN;

FairMQTransportFactoryNN::FairMQTransportFactoryNN(const string& id, const FairMQProgOptions* /*config*/)
    : FairMQTransportFactory(id)
{
    LOG(debug) << "Transport: Using nanomsg library";
}

FairMQMessagePtr FairMQTransportFactoryNN::CreateMessage()
{
    return unique_ptr<FairMQMessage>(new FairMQMessageNN(this));
}

FairMQMessagePtr FairMQTransportFactoryNN::CreateMessage(const size_t size)
{
    return unique_ptr<FairMQMessage>(new FairMQMessageNN(size, this));
}

FairMQMessagePtr FairMQTransportFactoryNN::CreateMessage(void* data, const size_t size, fairmq_free_fn* ffn, void* hint)
{
    return unique_ptr<FairMQMessage>(new FairMQMessageNN(data, size, ffn, hint, this));
}

FairMQMessagePtr FairMQTransportFactoryNN::CreateMessage(FairMQUnmanagedRegionPtr& region, void* data, const size_t size, void* hint)
{
    return unique_ptr<FairMQMessage>(new FairMQMessageNN(region, data, size, hint, this));
}

FairMQSocketPtr FairMQTransportFactoryNN::CreateSocket(const string& type, const string& name) const
{
    unique_ptr<FairMQSocket> socket(new FairMQSocketNN(type, name, GetId()));
    fSockets.push_back(socket.get());
    return socket;
}

FairMQPollerPtr FairMQTransportFactoryNN::CreatePoller(const vector<FairMQChannel>& channels) const
{
    return unique_ptr<FairMQPoller>(new FairMQPollerNN(channels));
}

FairMQPollerPtr FairMQTransportFactoryNN::CreatePoller(const std::vector<FairMQChannel*>& channels) const
{
    return unique_ptr<FairMQPoller>(new FairMQPollerNN(channels));
}

FairMQPollerPtr FairMQTransportFactoryNN::CreatePoller(const unordered_map<string, vector<FairMQChannel>>& channelsMap, const vector<string>& channelList) const
{
    return unique_ptr<FairMQPoller>(new FairMQPollerNN(channelsMap, channelList));
}

FairMQUnmanagedRegionPtr FairMQTransportFactoryNN::CreateUnmanagedRegion(const size_t size, FairMQRegionCallback callback) const
{
    return unique_ptr<FairMQUnmanagedRegion>(new FairMQUnmanagedRegionNN(size, callback));
}

fair::mq::Transport FairMQTransportFactoryNN::GetType() const
{
    return fTransportType;
}

void FairMQTransportFactoryNN::Reset()
{
    auto it = max_element(fSockets.begin(), fSockets.end(), [](FairMQSocket* s1, FairMQSocket* s2) {
        return static_cast<FairMQSocketNN*>(s1)->GetLinger() < static_cast<FairMQSocketNN*>(s2)->GetLinger();
    });
    if (it != fSockets.end()) {
        this_thread::sleep_for(chrono::milliseconds(static_cast<FairMQSocketNN*>(*it)->GetLinger()));
    }
    fSockets.clear();
}

FairMQTransportFactoryNN::~FairMQTransportFactoryNN()
{
    // nn_term();
    // see https://www.freelists.org/post/nanomsg/Getting-rid-of-nn-init-and-nn-term,8
}
