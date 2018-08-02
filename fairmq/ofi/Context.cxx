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

#include <arpa/inet.h>
#include <boost/version.hpp>
#include <cstring>
#include <memory>
#include <netinet/in.h>
#include <rdma/fabric.h>
#include <rdma/fi_domain.h>
#include <rdma/fi_endpoint.h>
#include <rdma/fi_errno.h>
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
    : fOfiDomain(nullptr)
    , fOfiFabric(nullptr)
    , fOfiInfo(nullptr)
    , fOfiAddressVector(nullptr)
    , fOfiEventQueue(nullptr)
    , fZmqContext(zmq_ctx_new())
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
            LOG(debug) << "I/O thread #" << i << "/" << numberIoThreads << " started";
            fIoContext.run();
            LOG(debug) << "I/O thread #" << i << "/" << numberIoThreads << " stopped";
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

	if (fOfiEventQueue) {
        auto ret = fi_close(&fOfiEventQueue->fid);
        if (ret != FI_SUCCESS)
            LOG(error) << "Failed closing ofi event queue, reason: " << fi_strerror(ret);
    }

	if (fOfiAddressVector) {
        auto ret = fi_close(&fOfiAddressVector->fid);
        if (ret != FI_SUCCESS)
            LOG(error) << "Failed closing ofi address vector, reason: " << fi_strerror(ret);
    }

	if (fOfiDomain) {
        auto ret = fi_close(&fOfiDomain->fid);
        if (ret != FI_SUCCESS)
            LOG(error) << "Failed closing ofi domain, reason: " << fi_strerror(ret);
    }

	if (fOfiFabric) {
        auto ret = fi_close(&fOfiFabric->fid);
        if (ret != FI_SUCCESS)
            LOG(error) << "Failed closing ofi fabric, reason: " << fi_strerror(ret);
    }
}

auto Context::GetZmqVersion() const -> string
{
    int major, minor, patch;
    zmq_version(&major, &minor, &patch);
    return tools::ToString(major, ".", minor, ".", patch);
}

auto Context::GetOfiApiVersion() const -> string
{
    // Disable for now, does not compile with gcc 4.9.2 debian jessie
    //auto ofi_version{fi_version()};
    //return tools::ToString(FI_MAJOR(ofi_version), ".", FI_MINOR(ofi_version));
    return "unknown";
}

auto Context::GetBoostVersion() const -> std::string
{
    return tools::ToString(BOOST_VERSION / 100000, ".", BOOST_VERSION / 100 % 1000, ".", BOOST_VERSION % 100);
}

auto Context::InitOfi(ConnectionType type, Address addr) -> void
{
    if (!fOfiInfo) {
        sockaddr_in* sa = static_cast<sockaddr_in*>(malloc(sizeof(sockaddr_in)));
        addr.Port = 0;
        auto sa2 = ConvertAddress(addr);
        memcpy(sa, &sa2, sizeof(sockaddr_in));

        // Prepare fi_getinfo query
        unique_ptr<fi_info, void(*)(fi_info*)> ofi_hints(fi_allocinfo(), fi_freeinfo);
        ofi_hints->caps = FI_MSG;
        //ofi_hints->mode = FI_CONTEXT;
        ofi_hints->addr_format = FI_SOCKADDR_IN;
        if (addr.Protocol == "tcp") {
            ofi_hints->fabric_attr->prov_name = strdup("sockets");
        } else if (addr.Protocol == "verbs") {
            ofi_hints->fabric_attr->prov_name = strdup("verbs;ofi_rxm");
        }
        ofi_hints->ep_attr->type = FI_EP_RDM;
        //ofi_hints->domain_attr->mr_mode = FI_MR_BASIC | FI_MR_SCALABLE;
        ofi_hints->domain_attr->threading = FI_THREAD_SAFE;
        ofi_hints->domain_attr->control_progress = FI_PROGRESS_AUTO;
        ofi_hints->domain_attr->data_progress = FI_PROGRESS_AUTO;
        ofi_hints->tx_attr->op_flags = FI_COMPLETION;
        ofi_hints->rx_attr->op_flags = FI_COMPLETION;
        if (type == ConnectionType::Bind) {
            ofi_hints->src_addr = sa;
            ofi_hints->src_addrlen = sizeof(sockaddr_in);
            ofi_hints->dest_addr = nullptr;
            ofi_hints->dest_addrlen = 0;
        } else {
            ofi_hints->src_addr = nullptr;
            ofi_hints->src_addrlen = 0;
            ofi_hints->dest_addr = sa;
            ofi_hints->dest_addrlen = sizeof(sockaddr_in);
        }

        // Query fi_getinfo for fabric to use
        auto res = fi_getinfo(FI_VERSION(1, 5), nullptr, nullptr, 0, ofi_hints.get(), &fOfiInfo);
        if (res != 0) throw ContextError{tools::ToString("Failed querying fi_getinfo, reason: ", fi_strerror(res))};
        if (!fOfiInfo) throw ContextError{"Could not find any ofi compatible fabric."};

        // for(auto cursor{ofi_info}; cursor->next != nullptr; cursor = cursor->next) {
            // LOG(debug) << fi_tostr(fOfiInfo, FI_TYPE_INFO);
        // }
        //
    } else {
        LOG(debug) << "Ofi info already queried. Skipping.";
    }

    OpenOfiFabric();
//    OpenOfiEventQueue();
    OpenOfiDomain();
    OpenOfiAddressVector();
}

auto Context::OpenOfiFabric() -> void
{
    if (!fOfiFabric) {
        assert(fOfiInfo);
        fi_context ctx;
        auto ret = fi_fabric(fOfiInfo->fabric_attr, &fOfiFabric, &ctx);
        if (ret != FI_SUCCESS)
            throw ContextError{tools::ToString("Failed opening ofi fabric, reason: ", fi_strerror(ret))};
    } else {
        // TODO Check, if requested fabric matches existing one.
        // TODO Decide, if we want to support more than one fabric simultaneously.
        LOG(debug) << "Ofi fabric already opened. Skipping.";
    }
}

auto Context::OpenOfiDomain() -> void
{
    if (!fOfiDomain) {
        assert(fOfiInfo);
        assert(fOfiFabric);
        fi_context ctx;
        auto ret = fi_domain(fOfiFabric, fOfiInfo, &fOfiDomain, &ctx);
        if (ret != FI_SUCCESS)
            throw ContextError{tools::ToString("Failed opening ofi domain, reason: ", fi_strerror(ret))};
    } else {
        LOG(debug) << "Ofi domain already opened. Skipping.";
    }
}

auto Context::OpenOfiEventQueue() -> void
{
    fi_eq_attr eqAttr = {100, 0, FI_WAIT_UNSPEC, 0, nullptr};
    // size_t               size;      [> # entries for EQ <]
    // uint64_t             flags;     [> operation flags <]
    // enum fi_wait_obj     wait_obj;  [> requested wait object <]
    // int                  signaling_vector; [> interrupt affinity <]
    // struct fid_wait     *wait_set;  [> optional wait set <]
    fi_context ctx;
    auto ret = fi_eq_open(fOfiFabric, &eqAttr, &fOfiEventQueue, &ctx);
    if (ret != FI_SUCCESS)
        throw ContextError{tools::ToString("Failed opening ofi event queue, reason: ", fi_strerror(ret))};
}

auto Context::OpenOfiAddressVector() -> void
{
    if (!fOfiAddressVector) {
        assert(fOfiDomain);
        fi_av_attr attr = {fOfiInfo->domain_attr->av_type, 0, 1000, 0, nullptr, nullptr, 0};
        // enum fi_av_type  type;        [> type of AV <]
        // int              rx_ctx_bits; [> address bits to identify rx ctx <]
        // size_t           count;       [> # entries for AV <]
        // size_t           ep_per_node; [> # endpoints per fabric address <]
        // const char       *name;       [> system name of AV <]
        // void             *map_addr;   [> base mmap address <]
        // uint64_t         flags;       [> operation flags <]
        fi_context ctx;
        auto ret = fi_av_open(fOfiDomain, &attr, &fOfiAddressVector, &ctx);
        if (ret != FI_SUCCESS)
            throw ContextError{tools::ToString("Failed opening ofi address vector, reason: ", fi_strerror(ret))};

        //assert(fOfiEventQueue);
        //ret = fi_av_bind(fOfiAddressVector, &fOfiEventQueue->fid, 0);
        //if (ret != FI_SUCCESS)
        //    throw ContextError{tools::ToString("Failed binding ofi event queue to address vector, reason: ", fi_strerror(ret))};
    } else {
        LOG(debug) << "Ofi address vector already opened. Skipping.";
    }
}

auto Context::CreateOfiEndpoint() -> fid_ep*
{
    assert(fOfiDomain);
    assert(fOfiInfo);
    fid_ep* ep = nullptr;
    fi_context ctx;
    auto ret = fi_endpoint(fOfiDomain, fOfiInfo, &ep, &ctx);
    if (ret != FI_SUCCESS)
        throw ContextError{tools::ToString("Failed creating ofi endpoint, reason: ", fi_strerror(ret))};

    //assert(fOfiEventQueue);
    //ret = fi_ep_bind(ep, &fOfiEventQueue->fid, 0);
    //if (ret != FI_SUCCESS)
    //    throw ContextError{tools::ToString("Failed binding ofi event queue to ofi endpoint, reason: ", fi_strerror(ret))};

    assert(fOfiAddressVector);
    ret = fi_ep_bind(ep, &fOfiAddressVector->fid, 0);
    if (ret != FI_SUCCESS)
        throw ContextError{tools::ToString("Failed binding ofi address vector to ofi endpoint, reason: ", fi_strerror(ret))};

    return ep;
}

auto Context::CreateOfiCompletionQueue(Direction dir) -> fid_cq*
{
    fid_cq* cq = nullptr;
    fi_cq_attr attr = {0, 0, FI_CQ_FORMAT_DATA, FI_WAIT_UNSPEC, 0, FI_CQ_COND_NONE, nullptr};
    if (dir == Direction::Receive) {
        attr.size = fOfiInfo->rx_attr->size;
    } else {
        attr.size = fOfiInfo->tx_attr->size;
    }
	// size_t               size;      [> # entries for CQ <]
	// uint64_t             flags;     [> operation flags <]
	// enum fi_cq_format    format;    [> completion format <]
	// enum fi_wait_obj     wait_obj;  [> requested wait object <]
	// int                  signaling_vector; [> interrupt affinity <]
	// enum fi_cq_wait_cond wait_cond; [> wait condition format <]
	// struct fid_wait     *wait_set;  [> optional wait set <]
    fi_context ctx;
    auto ret = fi_cq_open(fOfiDomain, &attr, &cq, &ctx);
    if (ret != FI_SUCCESS)
        throw ContextError{tools::ToString("Failed creating ofi completion queue, reason: ", fi_strerror(ret))};
    return cq;
}

auto Context::InsertAddressVector(sockaddr_in address) -> fi_addr_t
{
    fi_addr_t mappedAddress;
    fi_context ctx;
    auto ret = fi_av_insert(fOfiAddressVector, &address, 1, &mappedAddress, 0, &ctx);
    if (ret != 1)
        throw ContextError{tools::ToString("Failed to insert address into ofi address vector")};

    return mappedAddress;
}

auto Context::AddressVectorLookup(fi_addr_t address) -> sockaddr_in
{
    throw ContextError("Not yet implemented");
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

} /* namespace ofi */
} /* namespace mq */
} /* namespace fair */
