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

namespace fair
{
namespace mq
{
namespace ofi
{

using namespace std;

Context::Context(FairMQTransportFactory& sendFactory,
                 FairMQTransportFactory& receiveFactory,
                 int numberIoThreads)
    : fIoWork(fIoContext)
    , fReceiveFactory(receiveFactory)
    , fSendFactory(sendFactory)
{
    InitThreadPool(numberIoThreads);
}

auto Context::InitThreadPool(int numberIoThreads) -> void
{
    assert(numberIoThreads > 0);

    for (int i = 1; i <= numberIoThreads; ++i) {
        fThreadPool.emplace_back([&, i, numberIoThreads]{
            try {
                LOG(debug) << "OFI transport: I/O thread #" << i << " of " << numberIoThreads << " started";
                fIoContext.run();
                LOG(debug) << "OFI transport: I/O thread #" << i << " of " << numberIoThreads << " stopped";
            } catch (const std::exception& e) {
                LOG(error) << "OFI transport: Uncaught exception in I/O thread #" << i << ": " << e.what();
            } catch (...) {
                LOG(error) << "OFI transport: Uncaught exception in I/O thread #" << i;
            }
        });
    }
}

auto Context::Reset() -> void
{
    // TODO "Linger", rethink this
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    fIoContext.stop();
}

Context::~Context()
{
    for (auto& thread : fThreadPool)
        thread.join();
}

auto Context::GetAsiofiVersion() const -> string
{
    return ASIOFI_VERSION;
}

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

auto Context::MakeReceiveMessage(size_t size) -> MessagePtr
{
    return fReceiveFactory.CreateMessage(size);
}

auto Context::MakeSendMessage(size_t size) -> MessagePtr
{
    return fSendFactory.CreateMessage(size);
}

} /* namespace ofi */
} /* namespace mq */
} /* namespace fair */
