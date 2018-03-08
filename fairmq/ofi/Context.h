/********************************************************************************
 *    Copyright (C) 2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_OFI_CONTEXT_H
#define FAIR_MQ_OFI_CONTEXT_H

#include <boost/asio.hpp>
#include <memory>
#include <netinet/in.h>
#include <ostream>
#include <rdma/fabric.h>
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
    Context(int numberIoThreads = 2);
    ~Context();

    auto CreateOfiEndpoint() -> fid_ep*;
    auto CreateOfiCompletionQueue(Direction dir) -> fid_cq*;
    auto GetZmqVersion() const -> std::string;
    auto GetOfiApiVersion() const -> std::string;
    auto GetPbVersion() const -> std::string;
    auto GetBoostVersion() const -> std::string;
    auto GetZmqContext() const -> void* { return fZmqContext; }
    auto GetIoContext() -> boost::asio::io_service& { return fIoContext; }
    auto InsertAddressVector(sockaddr_in address) -> fi_addr_t;
    auto AddressVectorLookup(fi_addr_t address) -> sockaddr_in;
    struct Address {
        std::string Protocol;
        std::string Ip;
        unsigned int Port;
        friend auto operator<<(std::ostream& os, const Address& a) -> std::ostream& { return os << a.Protocol << "://" << a.Ip << ":" << a.Port; }
    };
    auto InitOfi(ConnectionType type, Address address) -> void;
    static auto ConvertAddress(std::string address) -> Address;
    static auto ConvertAddress(Address address) -> sockaddr_in;
    static auto ConvertAddress(sockaddr_in address) -> Address;
    static auto VerifyAddress(const std::string& address) -> Address;

  private:
    void* fZmqContext;
    fi_info* fOfiInfo;
    fid_fabric* fOfiFabric;
    fid_domain* fOfiDomain;
    fid_av* fOfiAddressVector;
    fid_eq* fOfiEventQueue;
    boost::asio::io_service fIoContext;
    boost::asio::io_service::work fIoWork;
    std::vector<std::thread> fThreadPool;

    auto OpenOfiFabric() -> void;
    auto OpenOfiEventQueue() -> void;
    auto OpenOfiDomain() -> void;
    auto OpenOfiAddressVector() -> void;
    auto InitThreadPool(int numberIoThreads) -> void;
}; /* class Context */

struct ContextError : std::runtime_error { using std::runtime_error::runtime_error; };

} /* namespace ofi */
} /* namespace mq */
} /* namespace fair */

#endif /* FAIR_MQ_OFI_CONTEXT_H */
