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

FairMQMessage* FairMQTransportFactoryZMQ::CreateMessage()
{
    return new FairMQMessageZMQ();
}

FairMQMessage* FairMQTransportFactoryZMQ::CreateMessage(const size_t size)
{
    return new FairMQMessageZMQ(size);
}

FairMQMessage* FairMQTransportFactoryZMQ::CreateMessage(void* data, const size_t size, fairmq_free_fn* ffn, void* hint)
{
    return new FairMQMessageZMQ(data, size, ffn, hint);
}

FairMQSocket* FairMQTransportFactoryZMQ::CreateSocket(const string& type, const std::string& name, const int numIoThreads, const std::string& id /*= ""*/)
{
    return new FairMQSocketZMQ(type, name, numIoThreads, id);
}

FairMQPoller* FairMQTransportFactoryZMQ::CreatePoller(const vector<FairMQChannel>& channels)
{
    return new FairMQPollerZMQ(channels);
}

FairMQPoller* FairMQTransportFactoryZMQ::CreatePoller(const std::unordered_map<std::string, std::vector<FairMQChannel>>& channelsMap, const std::initializer_list<std::string> channelList)
{
    return new FairMQPollerZMQ(channelsMap, channelList);
}

FairMQPoller* FairMQTransportFactoryZMQ::CreatePoller(const FairMQSocket& cmdSocket, const FairMQSocket& dataSocket)
{
    return new FairMQPollerZMQ(cmdSocket, dataSocket);
}
