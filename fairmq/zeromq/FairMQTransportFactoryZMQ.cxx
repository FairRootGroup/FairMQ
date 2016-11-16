/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQTransportFactoryZMQ.cxx
 *
 * @since 2014-01-20
 * @author: A. Rybalchenko
 */

#include "zmq.h"

#include "FairMQTransportFactoryZMQ.h"

using namespace std;

FairMQTransportFactoryZMQ::FairMQTransportFactoryZMQ()
{
    int major, minor, patch;
    zmq_version(&major, &minor, &patch);
    LOG(DEBUG) << "Using ZeroMQ library, version: " << major << "." << minor << "." << patch;
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

FairMQSocketPtr FairMQTransportFactoryZMQ::CreateSocket(const string& type, const string& name, const int numIoThreads, const string& id /*= ""*/) const
{
    return unique_ptr<FairMQSocket>(new FairMQSocketZMQ(type, name, numIoThreads, id));
}

FairMQPollerPtr FairMQTransportFactoryZMQ::CreatePoller(const vector<FairMQChannel>& channels) const
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
