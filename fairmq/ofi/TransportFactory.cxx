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

TransportFactory::TransportFactory(const string& id, const FairMQProgOptions* /*config*/)
try : FairMQTransportFactory{id}
{
    LOG(debug) << "OFI transport: Using AZMQ & "
               << "asiofi (" << fContext.GetAsiofiVersion() << ")";
}
catch (ContextError& e)
{
    throw TransportFactoryError{e.what()};
}

auto TransportFactory::CreateMessage() -> MessagePtr
{
    return MessagePtr{new Message(&fMemoryResource)};
}

auto TransportFactory::CreateMessage(const size_t size) -> MessagePtr
{
    return MessagePtr{new Message(&fMemoryResource, size)};
}

auto TransportFactory::CreateMessage(void* data,
                                     const size_t size,
                                     fairmq_free_fn* ffn,
                                     void* hint) -> MessagePtr
{
    return MessagePtr{new Message(&fMemoryResource, data, size, ffn, hint)};
}

auto TransportFactory::CreateMessage(UnmanagedRegionPtr& region,
                                     void* data,
                                     const size_t size,
                                     void* hint) -> MessagePtr
{
    return MessagePtr{new Message(&fMemoryResource, region, data, size, hint)};
}

auto TransportFactory::CreateSocket(const string& type, const string& name) -> SocketPtr
{
    return SocketPtr{new Socket(fContext, type, name, GetId(), this)};
}

auto TransportFactory::CreatePoller(const vector<FairMQChannel>& /*channels*/) const -> PollerPtr
{
    throw runtime_error{"Not yet implemented (Poller)."};
    // return PollerPtr{new Poller(channels)};
}

auto TransportFactory::CreatePoller(const vector<FairMQChannel*>& /*channels*/) const -> PollerPtr
{
    throw runtime_error{"Not yet implemented (Poller)."};
    // return PollerPtr{new Poller(channels)};
}

auto TransportFactory::CreatePoller(const unordered_map<string, vector<FairMQChannel>>& /*channelsMap*/, const vector<string>& /*channelList*/) const -> PollerPtr
{
    throw runtime_error{"Not yet implemented (Poller)."};
    // return PollerPtr{new Poller(channelsMap, channelList)};
}

auto TransportFactory::CreateUnmanagedRegion(const size_t /*size*/, FairMQRegionCallback /*callback*/) const -> UnmanagedRegionPtr
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
