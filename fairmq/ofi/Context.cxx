/********************************************************************************
 *    Copyright (C) 2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <fairmq/ofi/Context.h>
#include <fairmq/Tools.h>
#include <FairMQLogger.h>

#include <asiofi/version.hpp>
#include <arpa/inet.h>
#include <boost/version.hpp>
#include <cassert>
#include <cstring>
#include <memory>
#include <netinet/in.h>
#include <regex>
#include <string>
#include <string.h>
#include <sys/socket.h>
#include <zmq.h>

namespace fair
{
namespace mq
{
namespace ofi
{

using namespace std;

Context::Context(int numberIoThreads)
    : fZmqContext(zmq_ctx_new())
    , fOfiInfo(nullptr)
    , fOfiFabric(nullptr)
    , fOfiDomain(nullptr)
    , fIoWork(fIoContext)
{
    if (!fZmqContext)
        throw ContextError{tools::ToString("Failed creating zmq context, reason: ", zmq_strerror(errno))};

    InitThreadPool(numberIoThreads);
}

auto Context::InitThreadPool(int numberIoThreads) -> void
{
    assert(numberIoThreads > 0);

    for (int i = 1; i <= numberIoThreads; ++i) {
        fThreadPool.emplace_back([&, i, numberIoThreads]{
            LOG(debug) << "OFI transport: I/O thread #" << i << " of " << numberIoThreads << " started";
            fIoContext.run();
            LOG(debug) << "OFI transport: I/O thread #" << i << " of " << numberIoThreads << " stopped";
        });
    }
}

Context::~Context()
{
    fIoContext.stop();
    for (auto& thread : fThreadPool)
        thread.join();

    if (zmq_ctx_term(fZmqContext) != 0)
        LOG(error) << "Failed closing zmq context, reason: " << zmq_strerror(errno);
}

auto Context::GetZmqVersion() const -> string
{
    int major, minor, patch;
    zmq_version(&major, &minor, &patch);
    return tools::ToString(major, ".", minor, ".", patch);
}

auto Context::GetAsiofiVersion() const -> string
{
    return ASIOFI_VERSION;
}

auto Context::InitOfi(ConnectionType type, Address addr) -> void
{
    assert(!fOfiInfo);
    assert(!fOfiFabric);
    assert(!fOfiDomain);

    asiofi::hints hints;
    if (addr.Protocol == "tcp") {
        hints.set_provider("sockets");
    } else if (addr.Protocol == "verbs") {
        hints.set_provider("verbs");
    }
    if (type == ConnectionType::Bind) {
        fOfiInfo = tools::make_unique<asiofi::info>(addr.Ip.c_str(), std::to_string(addr.Port).c_str(), FI_SOURCE, hints);
    } else {
        fOfiInfo = tools::make_unique<asiofi::info>(addr.Ip.c_str(), std::to_string(addr.Port).c_str(), 0, hints);
    }
    LOG(debug) << "OFI transport: " << *fOfiInfo;

    fOfiFabric = tools::make_unique<asiofi::fabric>(*fOfiInfo);

    fOfiDomain = tools::make_unique<asiofi::domain>(*fOfiFabric);
}

auto Context::MakeOfiPassiveEndpoint(Address addr) -> unique_ptr<asiofi::passive_endpoint>
{
    InitOfi(ConnectionType::Bind, addr);

    return tools::make_unique<asiofi::passive_endpoint>(fIoContext, *fOfiFabric);
}

auto Context::MakeOfiConnectedEndpoint(const asiofi::info& info) -> std::unique_ptr<asiofi::connected_endpoint>
{
    return tools::make_unique<asiofi::connected_endpoint>(fIoContext, *fOfiDomain, info);
}

auto Context::MakeOfiConnectedEndpoint(Address addr) -> std::unique_ptr<asiofi::connected_endpoint>
{
    InitOfi(ConnectionType::Connect, addr);

    return tools::make_unique<asiofi::connected_endpoint>(fIoContext, *fOfiDomain);
}
// auto Context::CreateOfiEndpoint() -> fid_ep*
// {
    // assert(fOfiDomain);
    // assert(fOfiInfo);
    // fid_ep* ep = nullptr;
    // fi_context ctx;
    // auto ret = fi_endpoint(fOfiDomain, fOfiInfo, &ep, &ctx);
    // if (ret != FI_SUCCESS)
        // throw ContextError{tools::ToString("Failed creating ofi endpoint, reason: ", fi_strerror(ret))};

    //assert(fOfiEventQueue);
    //ret = fi_ep_bind(ep, &fOfiEventQueue->fid, 0);
    //if (ret != FI_SUCCESS)
    //    throw ContextError{tools::ToString("Failed binding ofi event queue to ofi endpoint, reason: ", fi_strerror(ret))};

    // assert(fOfiAddressVector);
    // ret = fi_ep_bind(ep, &fOfiAddressVector->fid, 0);
    // if (ret != FI_SUCCESS)
        // throw ContextError{tools::ToString("Failed binding ofi address vector to ofi endpoint, reason: ", fi_strerror(ret))};
//
    // return ep;
// }

auto Context::ConvertAddress(std::string address) -> Address
{
    string protocol, ip;
    unsigned int port = 0;
    regex address_regex("^([a-z]+)://([0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+):([0-9]+).*");
    smatch address_result;
    if (regex_match(address, address_result, address_regex)) {
        protocol = address_result[1];
        ip = address_result[2];
        port = stoul(address_result[3]);
        // LOG(debug) << "Parsed '" << protocol << "', '" << ip << "', '" << port << "' fields from '" << address << "'";
    } else {
        throw ContextError(tools::ToString("Wrong format: Address must be in format prot://ip:port"));
    }

    return { protocol, ip, port };
}

auto Context::ConvertAddress(Address address) -> sockaddr_in
{
    sockaddr_in sa;
    if (inet_pton(AF_INET, address.Ip.c_str(), &(sa.sin_addr)) != 1)
        throw ContextError(tools::ToString("Failed to convert given IP '", address.Ip, "' to struct in_addr, reason: ", strerror(errno)));
    sa.sin_port = htons(address.Port);
    sa.sin_family = AF_INET;

    return sa;
}

auto Context::ConvertAddress(sockaddr_in address) -> Address
{
    return {"tcp", inet_ntoa(address.sin_addr), ntohs(address.sin_port)};
}

auto Context::VerifyAddress(const std::string& address) -> Address
{
    auto addr = ConvertAddress(address);

    if (!(addr.Protocol == "tcp" || addr.Protocol == "verbs"))
        throw ContextError("Wrong protocol: Supported protocols are: tcp:// and verbs://");

    return addr;
}

} /* namespace ofi */
} /* namespace mq */
} /* namespace fair */
