/********************************************************************************
 *    Copyright (C) 2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_OFI_CONTROLMESSAGES_H
#define FAIR_MQ_OFI_CONTROLMESSAGES_H

#include <FairMQLogger.h>
#include <boost/asio/buffer.hpp>
#include <cstdint>
#include <functional>
#include <memory>
#include <type_traits>

namespace boost {
namespace asio {

template<typename PodType>
auto buffer(const PodType& obj) -> boost::asio::const_buffer
{
    return boost::asio::const_buffer(static_cast<const void*>(&obj), sizeof(PodType));
}

}   // namespace asio
}   // namespace boost

namespace fair {
namespace mq {
namespace ofi {

enum class ControlMessageType
{
    DataAddressAnnouncement = 1,
    PostBuffer,
    PostBufferAcknowledgement
};

struct ControlMessage
{
    ControlMessageType type;
};

struct DataAddressAnnouncement : ControlMessage
{
    uint32_t ipv4;   // in_addr_t from <netinet/in.h>
    uint32_t port;   // in_port_t from <netinet/in.h>
};

struct PostBuffer : ControlMessage
{
    uint64_t size;   // buffer size (size_t)
};

template<typename T, typename... Args>
auto MakeControlMessage(Args&&... args) -> T
{
    T ctrl = T(std::forward<Args>(args)...);

    if (std::is_same<T, DataAddressAnnouncement>::value) {
        ctrl.type = ControlMessageType::DataAddressAnnouncement;
    } else if (std::is_same<T, PostBuffer>::value) {
        ctrl.type = ControlMessageType::PostBuffer;
    }

    return ctrl;
}

}   // namespace ofi
}   // namespace mq
}   // namespace fair

#endif /* FAIR_MQ_OFI_CONTROLMESSAGES_H */ 
