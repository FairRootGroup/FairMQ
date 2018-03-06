/********************************************************************************
 *    Copyright (C) 2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <fairmq/ofi/Message.h>
#include <fairmq/ofi/Poller.h>
#include <fairmq/ofi/Socket.h>
#include <fairmq/ofi/TransportFactory.h>
#include <fairmq/Tools.h>

#include <stdexcept>

namespace fair
{
namespace mq
{
namespace ofi
{

using namespace std;

TransportFactory::TransportFactory(const string& id, const FairMQProgOptions* config)
try : FairMQTransportFactory{id}
{
    LOG(debug) << "Transport: Using ZeroMQ (" << fContext.GetZmqVersion() << ") & "
               << "OFI libfabric (API " << fContext.GetOfiApiVersion() << ") & "
               << "Google Protobuf (" << fContext.GetPbVersion() << ") & "
               << "Boost.Asio (" << fContext.GetBoostVersion() << ")";
}
catch (ContextError& e)
{
    throw TransportFactoryError{e.what()};
}

auto TransportFactory::CreateMessage() const -> MessagePtr
{
    return MessagePtr{new Message()};
}

auto TransportFactory::CreateMessage(const size_t size) const -> MessagePtr
{
    return MessagePtr{new Message(size)};
}

auto TransportFactory::CreateMessage(void* data, const size_t size, fairmq_free_fn* ffn, void* hint) const -> MessagePtr
{
    return MessagePtr{new Message(data, size, ffn, hint)};
}

auto TransportFactory::CreateMessage(UnmanagedRegionPtr& region, void* data, const size_t size, void* hint) const -> MessagePtr
{
    return MessagePtr{new Message(region, data, size, hint)};
}

auto TransportFactory::CreateSocket(const string& type, const string& name) const -> SocketPtr
{
    return SocketPtr{new Socket(fContext, type, name, GetId())};
}

auto TransportFactory::CreatePoller(const vector<FairMQChannel>& channels) const -> PollerPtr
{
    return PollerPtr{new Poller(channels)};
}

auto TransportFactory::CreatePoller(const vector<const FairMQChannel*>& channels) const -> PollerPtr
{
    return PollerPtr{new Poller(channels)};
}

auto TransportFactory::CreatePoller(const unordered_map<string, vector<FairMQChannel>>& channelsMap, const vector<string>& channelList) const -> PollerPtr
{
    return PollerPtr{new Poller(channelsMap, channelList)};
}

auto TransportFactory::CreatePoller(const FairMQSocket& cmdSocket, const FairMQSocket& dataSocket) const -> PollerPtr
{
    return PollerPtr{new Poller(cmdSocket, dataSocket)};
}

auto TransportFactory::CreateUnmanagedRegion(const size_t size, FairMQRegionCallback callback) const -> UnmanagedRegionPtr
{
    throw runtime_error{"Not yet implemented UMR."};
}

auto TransportFactory::GetType() const -> Transport
{
    return Transport::OFI;
}

} /* namespace ofi */
} /* namespace mq */
} /* namespace fair */
