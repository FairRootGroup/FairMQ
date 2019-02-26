/********************************************************************************
 *    Copyright (C) 2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_OFI_CONTEXT_H
#define FAIR_MQ_OFI_CONTEXT_H

#include <FairMQLogger.h>

#include <asiofi/connected_endpoint.hpp>
#include <asiofi/domain.hpp>
#include <asiofi/fabric.hpp>
#include <asiofi/info.hpp>
#include <asiofi/passive_endpoint.hpp>
#include <boost/asio/io_context.hpp>
#include <memory>
#include <netinet/in.h>
#include <ostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace fair
{
namespace mq
{
namespace ofi
{

enum class ConnectionType : bool { Bind, Connect };
enum class Direction : bool { Receive, Transmit };

/**
 * @class Context Context.h <fairmq/ofi/Context.h>
 * @brief Transport-wide context
 *
 * @todo TODO insert long description
 */
class Context
{
  public:
    Context(int numberIoThreads = 1);
    ~Context();

    // auto CreateOfiEndpoint() -> fid_ep*;
    auto GetAsiofiVersion() const -> std::string;
    auto GetIoContext() -> boost::asio::io_context& { return fIoContext; }
    struct Address {
        std::string Protocol;
        std::string Ip;
        unsigned int Port;
        friend auto operator<<(std::ostream& os, const Address& a) -> std::ostream& { return os << a.Protocol << "://" << a.Ip << ":" << a.Port; }
    };
    auto MakeOfiPassiveEndpoint(Address addr) -> std::unique_ptr<asiofi::passive_endpoint>;
    auto MakeOfiConnectedEndpoint(Address addr) -> std::unique_ptr<asiofi::connected_endpoint>;
    auto MakeOfiConnectedEndpoint(const asiofi::info& info) -> std::unique_ptr<asiofi::connected_endpoint>;
    static auto ConvertAddress(std::string address) -> Address;
    static auto ConvertAddress(Address address) -> sockaddr_in;
    static auto ConvertAddress(sockaddr_in address) -> Address;
    static auto VerifyAddress(const std::string& address) -> Address;
    auto GetDomain() const -> const asiofi::domain& { return *fOfiDomain; }
    auto Interrupt() -> void { LOG(debug) << "OFI transport: Interrupted (NOOP - not implemented)."; }
    auto Resume() -> void { LOG(debug) << "OFI transport: Resumed (NOOP - not implemented)."; }

  private:
    std::unique_ptr<asiofi::info> fOfiInfo;
    std::unique_ptr<asiofi::fabric> fOfiFabric;
    std::unique_ptr<asiofi::domain> fOfiDomain;
    boost::asio::io_context fIoContext;
    boost::asio::io_context::work fIoWork;
    std::vector<std::thread> fThreadPool;

    auto InitThreadPool(int numberIoThreads) -> void;
    auto InitOfi(ConnectionType type, Address address) -> void;
}; /* class Context */

struct ContextError : std::runtime_error { using std::runtime_error::runtime_error; };

} /* namespace ofi */
} /* namespace mq */
} /* namespace fair */

#endif /* FAIR_MQ_OFI_CONTEXT_H */
