/********************************************************************************
 *    Copyright (C) 2018 CERN and copyright holders of ALICE O2                 *
 *    Copyright (C) 2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

/// @brief Tools for interfacing containers to the transport via polymorphic
/// allocators
///
/// @author Mikolaj Krzewicki, mkrzewic@cern.ch

#include <fairmq/FairMQTransportFactory.h>
#include <fairmq/MemoryResources.h>

namespace fair {
namespace mq {

using BytePmrAllocator = pmr::polymorphic_allocator<fair::mq::byte>;

//_________________________________________________________________________________________________
// return the message associated with the container or throw if it is not possible
template<typename ContainerT>
// typename std::enable_if<
//    std::is_base_of<
//        pmr::polymorphic_allocator<typename
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
        {
            message->SetUsedSize(containerSizeBytes);
            return message;
        } else {
          //container is not required to allocate (like in std::string small string optimization)
          //in case we get no message we fall back to default (copy) behaviour)
          targetResource = resource;
        }
    }

    auto message = targetResource->getTransportFactory()->CreateMessage(containerSizeBytes);
    std::memcpy(static_cast<fair::mq::byte *>(message->GetData()),
        container.data(),
        containerSizeBytes);
    return message;
}

} /* namespace mq */
} /* namespace fair */
