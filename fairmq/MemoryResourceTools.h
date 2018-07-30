/********************************************************************************
 *    Copyright (C) 2018 CERN and copyright holders of ALICE O2 *
 *    Copyright (C) 2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH *
 *                                                                              *
 *              This software is distributed under the terms of the *
 *              GNU Lesser General Public Licence (LGPL) version 3, *
 *                  copied verbatim in the file "LICENSE" *
 ********************************************************************************/

/// @brief Tools for interfacing containers to the transport via polymorphic
/// allocators
///
/// @author Mikolaj Krzewicki, mkrzewic@cern.ch

#include <fairmq/FairMQTransportFactory.h>
#include <fairmq/MemoryResources.h>

namespace fair {
namespace mq {

using ByteSpectatorAllocator = SpectatorAllocator<fair::mq::byte>;
using BytePmrAllocator = boost::container::pmr::polymorphic_allocator<fair::mq::byte>;

//_________________________________________________________________________________________________
// return the message associated with the container or nullptr if it does not
// make sense (e.g. when
// we are just watching an existing message or when the container is not using
// FairMQMemoryResource
// as backend).
template<typename ContainerT>
// typename std::enable_if<
//    std::is_base_of<
//        boost::container::pmr::polymorphic_allocator<typename
//        ContainerT::value_type>,
//        typename ContainerT::allocator_type>::value == true,
//    FairMQMessagePtr>::type
FairMQMessagePtr getMessage(ContainerT &&container_, FairMQMemoryResource *targetResource = nullptr)
{
    auto container = std::move(container_);
    auto alloc = container.get_allocator();

    auto resource = dynamic_cast<FairMQMemoryResource *>(alloc.resource());
    if (!resource && !targetResource) {
        throw std::runtime_error("Neither the container or target resource specified");
    }
    size_t containerSizeBytes = container.size() * sizeof(typename ContainerT::value_type);
    if ((!targetResource && resource)
        || (resource && targetResource && resource->is_equal(*targetResource))) {
        auto message = resource->getMessage(static_cast<void *>(
            const_cast<typename std::remove_const<typename ContainerT::value_type>::type *>(
                container.data())));
        if (message)
            message->SetUsedSize(containerSizeBytes);
        return std::move(message);
    } else {
        auto message = targetResource->getTransportFactory()->CreateMessage(containerSizeBytes);
        std::memcpy(static_cast<fair::mq::byte *>(message->GetData()),
                    container.data(),
                    containerSizeBytes);
        return std::move(message);
    }
};

//_________________________________________________________________________________________________
/// Return a vector of const ElemT, resource must be kept alive throughout the
/// lifetime of the
/// container and associated message.
template<typename ElemT>
std::vector<const ElemT, boost::container::pmr::polymorphic_allocator<const ElemT>> adoptVector(
    size_t nelem,
    SpectatorMessageResource *resource)
{
    return std::vector<const ElemT, SpectatorAllocator<const ElemT>>(
        nelem, SpectatorAllocator<ElemT>(resource));
};

//_________________________________________________________________________________________________
/// Return a vector of const ElemT, takes ownership of the message, needs an
/// upstream global
/// ChannelResource to register the message.
template<typename ElemT>
std::vector<const ElemT, OwningMessageSpectatorAllocator<const ElemT>>
    adoptVector(size_t nelem, ChannelResource *upstream, FairMQMessagePtr message)
{
    return std::vector<const ElemT, OwningMessageSpectatorAllocator<const ElemT>>(
        nelem,
        OwningMessageSpectatorAllocator<const ElemT>(
            MessageResource{std::move(message), upstream}));
};

//_________________________________________________________________________________________________
// TODO: this is C++14, converting it down to C++11 is too much work atm
// This returns a unique_ptr of const vector, does not allow modifications at
// the cost of pointer
// semantics for access.
// use auto or decltype to catch the return value (or use span)
// template<typename ElemT>
// auto adoptVector(size_t nelem, FairMQMessage* message)
//{
//    using DataType = std::vector<ElemT, ByteSpectatorAllocator>;
//
//    struct doubleDeleter
//    {
//        // kids: don't do this at home! (but here it's OK)
//        // this stateful deleter allows a single unique_ptr to manage 2
//        resources at the same time.
//        std::unique_ptr<SpectatorMessageResource> extra;
//        void operator()(const DataType* ptr) { delete ptr; }
//    };
//
//    using OutputType = std::unique_ptr<const DataType, doubleDeleter>;
//
//    auto resource = std::make_unique<SpectatorMessageResource>(message);
//    auto output = new DataType(nelem, ByteSpectatorAllocator{resource.get()});
//    return OutputType(output, doubleDeleter{std::move(resource)});
//}

} /* namespace mq */
} /* namespace fair */
