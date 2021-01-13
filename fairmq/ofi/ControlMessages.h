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
#include <boost/container/pmr/memory_resource.hpp>
#include <cstdint>
#include <functional>
#include <memory>
#include <type_traits>

namespace boost::asio
{

template<typename PodType>
auto buffer(const PodType& obj) -> boost::asio::const_buffer
{
    return boost::asio::const_buffer(static_cast<const void*>(&obj), sizeof(PodType));
}

} // namespace boost::asio

namespace fair::mq::ofi
{

enum class ControlMessageType
{
    Empty = 1,
    PostBuffer,
    PostMultiPartStartBuffer
};

struct Empty
{};

struct PostBuffer
{
    uint64_t size;   // buffer size (size_t)
};

struct PostMultiPartStartBuffer
{
    uint32_t numParts;   // buffer size (size_t)
    uint64_t size;       // buffer size (size_t)
};

union ControlMessageContent
{
    PostBuffer postBuffer;
    PostMultiPartStartBuffer postMultiPartStartBuffer;
};

struct ControlMessage
{
    ControlMessageType type;
    ControlMessageContent msg;
};

template<typename T>
using unique_ptr = std::unique_ptr<T, std::function<void(T*)>>;

template<typename T, typename... Args>
auto MakeControlMessageWithPmr(boost::container::pmr::memory_resource& pmr, Args&&... args)
    -> ofi::unique_ptr<ControlMessage>
{
    void* mem = pmr.allocate(sizeof(ControlMessage));
    ControlMessage* ctrl = new (mem) ControlMessage();

    if (std::is_same<T, PostBuffer>::value) {
        ctrl->type = ControlMessageType::PostBuffer;
        ctrl->msg.postBuffer = PostBuffer(std::forward<Args>(args)...);
    } else if (std::is_same<T, PostMultiPartStartBuffer>::value) {
        ctrl->type = ControlMessageType::PostMultiPartStartBuffer;
        ctrl->msg.postMultiPartStartBuffer = PostMultiPartStartBuffer(std::forward<Args>(args)...);
    } else if (std::is_same<T, Empty>::value) {
        ctrl->type = ControlMessageType::Empty;
    }

    return ofi::unique_ptr<ControlMessage>(ctrl, [&pmr](ControlMessage* p) {
        p->~ControlMessage();
        pmr.deallocate(p, sizeof(T));
    });
}

template<typename T, typename... Args>
auto MakeControlMessage(Args&&... args) -> ControlMessage
{
    ControlMessage ctrl;

    if (std::is_same<T, PostBuffer>::value) {
        ctrl.type = ControlMessageType::PostBuffer;
    } else if (std::is_same<T, PostMultiPartStartBuffer>::value) {
        ctrl.type = ControlMessageType::PostMultiPartStartBuffer;
    } else if (std::is_same<T, Empty>::value) {
        ctrl.type = ControlMessageType::Empty;
    }
    ctrl.msg = T(std::forward<Args>(args)...);

    return ctrl;
}

} // namespace fair::mq::ofi

#endif /* FAIR_MQ_OFI_CONTROLMESSAGES_H */ 
