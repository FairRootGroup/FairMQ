/********************************************************************************
 *    Copyright (C) 2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_OFI_CONTROLMESSAGES_H
#define FAIR_MQ_OFI_CONTROLMESSAGES_H

#include <cstdint>
#include <functional>
#include <memory>
#include <type_traits>

namespace fair
{
namespace mq
{
namespace ofi
{

enum class ControlMessageType
{
    DataAddressAnnouncement = 1,
    PostBuffer,
    PostBufferAcknowledgement
};

struct ControlMessage {
    ControlMessageType type;
};

struct DataAddressAnnouncement : ControlMessage {
    uint32_t ipv4; // in_addr_t from <netinet/in.h>
    uint32_t port; // in_port_t from <netinet/in.h>
};

struct PostBuffer : ControlMessage {
    uint64_t size; // buffer size (size_t)
};

struct PostBufferAcknowledgement {
    uint64_t size; // size_t
};

template<typename T>
using CtrlMsgPtr = std::unique_ptr<T, std::function<void(T*)>>;

template<typename T, typename A, typename ... Args>
auto MakeControlMessage(A* pmr, Args&& ... args) -> CtrlMsgPtr<T> 
{
    void* raw_mem = pmr->allocate(sizeof(T));
    T* raw_ptr = new (raw_mem) T(std::forward<Args>(args)...);

    if (std::is_same<T, DataAddressAnnouncement>::value) {
        raw_ptr->type = ControlMessageType::DataAddressAnnouncement;
    }

    return {raw_ptr, [=](T* p) { pmr->deallocate(p, sizeof(T)); }};
}

template<typename Derived, typename Base, typename Del>
auto StaticUniquePtrDowncast(std::unique_ptr<Base, Del>&& p) -> std::unique_ptr<Derived, Del>
{
    auto down = static_cast<Derived*>(p.release());
    return std::unique_ptr<Derived, Del>(down, std::move(p.get_deleter()));
}

template<typename Base, typename Derived, typename Del>
auto StaticUniquePtrUpcast(std::unique_ptr<Derived, Del>&& p) -> std::unique_ptr<Base, std::function<void(Base*)>>
{
    auto up = static_cast<Base*>(p.release());
    return {up, [deleter = std::move(p.get_deleter())](Base* ptr) {
                deleter(static_cast<Derived*>(ptr));
            }};
}

} /* namespace ofi */
} /* namespace mq */
} /* namespace fair */

#endif /* FAIR_MQ_OFI_CONTROLMESSAGES_H */ 
