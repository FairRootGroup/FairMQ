/********************************************************************************
 *    Copyright (C) 2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <fairmq/ofi/Poller.h>
#include <fairmq/ofi/Socket.h>
#include <fairmq/ofi/TransportFactory.h>
#include <fairmq/Tools.h>

#include <rdma/fabric.h> // OFI libfabric
#include <rdma/fi_errno.h>
#include <stdexcept>
#include <zmq.h>

namespace fair
{
namespace mq
{
namespace ofi
{

using namespace std;

TransportFactory::TransportFactory(const string& id, const FairMQProgOptions* config)
    : FairMQTransportFactory{id}
    , fZmqContext{zmq_ctx_new()}
{
    if (!fZmqContext)
    {
        throw TransportFactoryError{tools::ToString("Failed creating zmq context, reason: ", zmq_strerror(errno))};
    }

    auto ofi_hints = fi_allocinfo();
    ofi_hints->caps = FI_MSG | FI_RMA;
    ofi_hints->mode = FI_ASYNC_IOV;
    ofi_hints->addr_format = FI_SOCKADDR_IN;
    auto ofi_info = fi_allocinfo();
    if (ofi_hints == nullptr || ofi_info == nullptr)
    {
        throw TransportFactoryError{"Failed allocating fi_info structs"};
    }
    auto res = fi_getinfo(FI_VERSION(1, 5), nullptr, nullptr, 0, ofi_hints, &ofi_info);
    if (res != 0)
    {
        throw TransportFactoryError{tools::ToString("Failed querying fi_getinfo, reason: ", fi_strerror(res))};
    }
    for(auto cursor{ofi_info}; cursor->next != nullptr; cursor = cursor->next)
    {
        LOG(debug) << fi_tostr(cursor, FI_TYPE_INFO);
    }
    fi_freeinfo(ofi_hints);
    fi_freeinfo(ofi_info);

    int major, minor, patch;
    zmq_version(&major, &minor, &patch);
    auto ofi_version{fi_version()};
    LOG(debug) << "Transport: Using ZeroMQ (" << major << "." << minor << "." << patch << ") & "
               << "OFI libfabric (API " << FI_MAJOR(ofi_version) << "." << FI_MINOR(ofi_version) << ")";
}

auto TransportFactory::CreateMessage() const -> MessagePtr
{
    throw runtime_error{"Not yet implemented Msg1."};
}

auto TransportFactory::CreateMessage(const size_t size) const -> MessagePtr
{
    throw runtime_error{"Not yet implemented Msg2."};
}

auto TransportFactory::CreateMessage(void* data, const size_t size, fairmq_free_fn* ffn, void* hint) const -> MessagePtr
{
    throw runtime_error{"Not yet implemented Msg3."};
}

auto TransportFactory::CreateMessage(UnmanagedRegionPtr& region, void* data, const size_t size, void* hint) const -> MessagePtr
{
    throw runtime_error{"Not yet implemented Msg4."};
}

auto TransportFactory::CreateSocket(const string& type, const string& name) const -> SocketPtr
{
    assert(fZmqContext);
    return unique_ptr<FairMQSocket>{new Socket(type, name, GetId(), fZmqContext)};
}

auto TransportFactory::CreatePoller(const vector<FairMQChannel>& channels) const -> PollerPtr
{
    return unique_ptr<FairMQPoller>(new Poller(channels));
}

auto TransportFactory::CreatePoller(const vector<const FairMQChannel*>& channels) const -> PollerPtr
{
    return unique_ptr<FairMQPoller>(new Poller(channels));
}

auto TransportFactory::CreatePoller(const unordered_map<string, vector<FairMQChannel>>& channelsMap, const vector<string>& channelList) const -> PollerPtr
{
    return unique_ptr<FairMQPoller>(new Poller(channelsMap, channelList));
}

auto TransportFactory::CreatePoller(const FairMQSocket& cmdSocket, const FairMQSocket& dataSocket) const -> PollerPtr
{
    return unique_ptr<FairMQPoller>(new Poller(cmdSocket, dataSocket));
}

auto TransportFactory::CreateUnmanagedRegion(const size_t size, FairMQRegionCallback callback) const -> UnmanagedRegionPtr
{
    throw runtime_error{"Not yet implemented UMR."};
}

auto TransportFactory::GetType() const -> Transport
{
    return Transport::OFI;
}

TransportFactory::~TransportFactory()
{
    if (zmq_ctx_term(fZmqContext) != 0) {
        throw TransportFactoryError{tools::ToString("Failed closing zmq context, reason: ", zmq_strerror(errno))};
    }
}

} /* namespace ofi */
} /* namespace mq */
} /* namespace fair */
