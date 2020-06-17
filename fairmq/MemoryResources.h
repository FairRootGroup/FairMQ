/********************************************************************************
 *    Copyright (C) 2018 CERN and copyright holders of ALICE O2                 *
 *    Copyright (C) 2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

/// @brief Memory allocators and interfaces related to managing memory via the
/// trasport layer
///
/// @author Mikolaj Krzewicki, mkrzewic@cern.ch

#ifndef FAIR_MQ_MEMORY_RESOURCES_H
#define FAIR_MQ_MEMORY_RESOURCES_H

#include <fairmq/FairMQMessage.h>
class FairMQTransportFactory;

#include <boost/container/flat_map.hpp>
#include <boost/container/pmr/memory_resource.hpp>
#include <boost/container/pmr/monotonic_buffer_resource.hpp>
#include <boost/container/pmr/polymorphic_allocator.hpp>
#include <cstring>
#include <utility>

namespace fair {
namespace mq {

using byte = unsigned char;
namespace pmr = boost::container::pmr;

/// All FairMQ related memory resources need to inherit from this interface
/// class for the
/// getMessage() api.
class FairMQMemoryResource : public pmr::memory_resource
{
  public:
    /// return the message containing data associated with the pointer (to start
    /// of
    /// buffer), e.g. pointer returned by std::vector::data() return nullptr if
    /// returning
    /// a message does not make sense!
    virtual FairMQMessagePtr getMessage(void *p) = 0;
    virtual void *setMessage(FairMQMessagePtr) = 0;
    virtual FairMQTransportFactory *getTransportFactory() noexcept = 0;
    virtual size_t getNumberOfMessages() const noexcept = 0;
};

/// This is the allocator that interfaces to FairMQ memory management. All
/// allocations are
/// delegated to FairMQ so standard (e.g. STL) containers can construct their
/// stuff in
/// memory regions appropriate for the data channel configuration.
class ChannelResource : public FairMQMemoryResource
{
  protected:
    FairMQTransportFactory *factory{nullptr};
    // TODO: for now a map to keep track of allocations, something else would
    // probably be
    // faster, but for now this does not need to be fast.
    boost::container::flat_map<void *, FairMQMessagePtr> messageMap;

  public:
    ChannelResource() = delete;

    ChannelResource(FairMQTransportFactory *_factory)
        : FairMQMemoryResource()
        , factory(_factory)
        , messageMap()
    {
        if (!_factory) {
            throw std::runtime_error("Tried to construct from a nullptr FairMQTransportFactory");
        }
    };

    FairMQMessagePtr getMessage(void *p) override
    {
        auto mes = std::move(messageMap[p]);
        messageMap.erase(p);
        return mes;
    }

    void *setMessage(FairMQMessagePtr message) override
    {
        void *addr = message->GetData();
        messageMap[addr] = std::move(message);
        return addr;
    }

    FairMQTransportFactory *getTransportFactory() noexcept override { return factory; }

    size_t getNumberOfMessages() const noexcept override { return messageMap.size(); }

  protected:
    void *do_allocate(std::size_t bytes, std::size_t alignment) override;
    void do_deallocate(void *p, std::size_t /*bytes*/, std::size_t /*alignment*/) override
    {
        messageMap.erase(p);
    };

    bool do_is_equal(const pmr::memory_resource &other) const noexcept override
    {
        return this == &other;
    };
};

} /* namespace mq */
} /* namespace fair */

#endif /* FAIR_MQ_MEMORY_RESOURCES_H */
