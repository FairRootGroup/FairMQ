/********************************************************************************
 * Copyright (C) 2018-2021 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_OFI_CONTEXT_H
#define FAIR_MQ_OFI_CONTEXT_H

#include <asio/io_context.hpp>
#include <asiofi/domain.hpp>
#include <asiofi/fabric.hpp>
#include <asiofi/info.hpp>
#include <fairlogger/Logger.h>
#include <fairmq/TransportFactory.h>
#include <memory>
#include <netinet/in.h>
#include <ostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace fair::mq::ofi
{

enum class ConnectionType : bool { Bind, Connect };

struct Address {
    std::string Protocol;
    std::string Ip;
    unsigned int Port;
    friend auto operator<<(std::ostream& os, const Address& a) -> std::ostream&
    {
        return os << a.Protocol << "://" << a.Ip << ":" << a.Port;
    }
    friend auto operator==(const Address& lhs, const Address& rhs) -> bool
    {
        return (lhs.Protocol == rhs.Protocol) && (lhs.Ip == rhs.Ip) && (lhs.Port == rhs.Port);
    }
};

/**
 * @class Context Context.h <fairmq/ofi/Context.h>
 * @brief Transport-wide context
 *
 * @todo TODO insert long description
 */
class Context
{
  public:
    Context(FairMQTransportFactory& sendFactory,
            FairMQTransportFactory& receiveFactory,
            int numberIoThreads = 1);
    Context(const Context&) = delete;
    Context(Context&&) = delete;
    Context& operator=(const Context&) = delete;
    Context& operator=(Context&&) = delete;
    ~Context();

    auto GetAsiofiVersion() const -> std::string;
    auto GetIoContext() -> asio::io_context& { return fIoContext; }
    static auto ConvertAddress(std::string address) -> Address;
    static auto ConvertAddress(Address address) -> sockaddr_in;
    static auto ConvertAddress(sockaddr_in address) -> Address;
    static auto VerifyAddress(const std::string& address) -> Address;
    auto Interrupt() -> void { LOG(debug) << "OFI transport: Interrupted (NOOP - not implemented)."; }
    auto Resume() -> void { LOG(debug) << "OFI transport: Resumed (NOOP - not implemented)."; }
    auto Reset() -> void;
    auto MakeReceiveMessage(size_t size) -> MessagePtr;
    auto MakeSendMessage(size_t size) -> MessagePtr;
    auto GetSizeHint() -> size_t { return fSizeHint; }
    auto SetSizeHint(size_t size) -> void { fSizeHint = size; }

  private:
    asio::io_context fIoContext;
    asio::io_context::work fIoWork;
    std::vector<std::thread> fThreadPool;
    FairMQTransportFactory& fReceiveFactory;
    FairMQTransportFactory& fSendFactory;
    size_t fSizeHint;

    auto InitThreadPool(int numberIoThreads) -> void;
}; /* class Context */

struct ContextError : std::runtime_error { using std::runtime_error::runtime_error; };

} // namespace fair::mq::ofi

#endif /* FAIR_MQ_OFI_CONTEXT_H */
