/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
#include "zmq.h"
#include <boost/version.hpp>

#include "FairMQTransportFactorySHM.h"

using namespace std;

static FairMQ::Transport gTransportType = FairMQ::Transport::SHM;

FairMQTransportFactorySHM::FairMQTransportFactorySHM()
{
    int major, minor, patch;
    zmq_version(&major, &minor, &patch);
    LOG(DEBUG) << "Transport: Using ZeroMQ (" << major << "." << minor << "." << patch << ") & "
               << "boost::interprocess (" << (BOOST_VERSION / 100000) << "." << (BOOST_VERSION / 100 % 1000) << "." << (BOOST_VERSION % 100) << ")";
}

FairMQMessagePtr FairMQTransportFactorySHM::CreateMessage() const
{
    return unique_ptr<FairMQMessage>(new FairMQMessageSHM());
}

FairMQMessagePtr FairMQTransportFactorySHM::CreateMessage(const size_t size) const
{
    return unique_ptr<FairMQMessage>(new FairMQMessageSHM(size));
}

FairMQMessagePtr FairMQTransportFactorySHM::CreateMessage(void* data, const size_t size, fairmq_free_fn* ffn, void* hint) const
{
    return unique_ptr<FairMQMessage>(new FairMQMessageSHM(data, size, ffn, hint));
}

FairMQSocketPtr FairMQTransportFactorySHM::CreateSocket(const string& type, const string& name, const int numIoThreads, const string& id /*= ""*/) const
{
    return unique_ptr<FairMQSocket>(new FairMQSocketSHM(type, name, numIoThreads, id));
}

FairMQPollerPtr FairMQTransportFactorySHM::CreatePoller(const vector<FairMQChannel>& channels) const
{
    return unique_ptr<FairMQPoller>(new FairMQPollerSHM(channels));
}

FairMQPollerPtr FairMQTransportFactorySHM::CreatePoller(const unordered_map<string, vector<FairMQChannel>>& channelsMap, const vector<string>& channelList) const
{
    return unique_ptr<FairMQPoller>(new FairMQPollerSHM(channelsMap, channelList));
}

FairMQPollerPtr FairMQTransportFactorySHM::CreatePoller(const FairMQSocket& cmdSocket, const FairMQSocket& dataSocket) const
{
    return unique_ptr<FairMQPoller>(new FairMQPollerSHM(cmdSocket, dataSocket));
}

FairMQ::Transport FairMQTransportFactorySHM::GetType() const
{
    return gTransportType;
}

