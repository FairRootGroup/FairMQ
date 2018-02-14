/********************************************************************************
 *    Copyright (C) 2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <fairmq/ofi/TransportFactory.h>
#include <stdexcept>

namespace fair
{
namespace mq
{
namespace ofi
{

using namespace std;

TransportFactory::TransportFactory(const string& id, const FairMQProgOptions* config)
: FairMQTransportFactory{id}
{
}

auto TransportFactory::CreateMessage() const -> MessagePtr
{
    throw runtime_error{"Not yet implemented."};
}

auto TransportFactory::CreateMessage(const size_t size) const -> MessagePtr
{
    throw runtime_error{"Not yet implemented."};
}

auto TransportFactory::CreateMessage(void* data, const size_t size, fairmq_free_fn* ffn, void* hint) const -> MessagePtr
{
    throw runtime_error{"Not yet implemented."};
}

auto TransportFactory::CreateMessage(UnmanagedRegionPtr& region, void* data, const size_t size, void* hint) const -> MessagePtr
{
    throw runtime_error{"Not yet implemented."};
}

auto TransportFactory::CreateSocket(const string& type, const string& name) const -> SocketPtr
{
    throw runtime_error{"Not yet implemented."};
}

auto TransportFactory::CreatePoller(const vector<FairMQChannel>& channels) const -> PollerPtr
{
    throw runtime_error{"Not yet implemented."};
}

auto TransportFactory::CreatePoller(const vector<const FairMQChannel*>& channels) const -> PollerPtr
{
    throw runtime_error{"Not yet implemented."};
}

auto TransportFactory::CreatePoller(const unordered_map<string, vector<FairMQChannel>>& channelsMap, const vector<string>& channelList) const -> PollerPtr
{
    throw runtime_error{"Not yet implemented."};
}

auto TransportFactory::CreatePoller(const FairMQSocket& cmdSocket, const FairMQSocket& dataSocket) const -> PollerPtr
{
    throw runtime_error{"Not yet implemented."};
}

auto TransportFactory::CreateUnmanagedRegion(const size_t size, FairMQRegionCallback callback) const -> UnmanagedRegionPtr
{
    throw runtime_error{"Not yet implemented."};
}

auto TransportFactory::GetType() const -> Transport
{
    return Transport::OFI;
}

TransportFactory::~TransportFactory()
{
}

} /* namespace ofi */
} /* namespace mq */
} /* namespace fair */
