/********************************************************************************
 *    Copyright (C) 2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_OFI_CONTEXT_H
#define FAIR_MQ_OFI_CONTEXT_H

#include <memory>
#include <netinet/in.h>
#include <rdma/fabric.h>
#include <string>
#include <stdexcept>

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

    /// Deferred Ofi initialization
    auto InitOfi(ConnectionType type, std::string address) -> void;
    auto CreateOfiEndpoint() -> fid_ep*;
    auto CreateOfiCompletionQueue(Direction dir) -> fid_cq*;
    auto GetZmqVersion() const -> std::string;
    auto GetOfiApiVersion() const -> std::string;
    auto GetZmqContext() const -> void* { return fZmqContext; }
    auto InsertAddressVector(sockaddr_in address) -> fi_addr_t;
    struct Address {
        std::string Protocol;
        std::string Ip;
        unsigned int Port;
    };
    static auto ConvertAddress(std::string address) -> Address;
    static auto ConvertAddress(Address address) -> sockaddr_in;

  private:
    void* fZmqContext;
    fi_info* fOfiInfo;
    fid_fabric* fOfiFabric;
    fid_domain* fOfiDomain;
    fid_av* fOfiAddressVector;

    auto OpenOfiFabric() -> void;
    auto OpenOfiDomain() -> void;
    auto OpenOfiAddressVector() -> void;
}; /* class Context */

struct ContextError : std::runtime_error { using std::runtime_error::runtime_error; };

} /* namespace ofi */
} /* namespace mq */
} /* namespace fair */

#endif /* FAIR_MQ_OFI_CONTEXT_H */
