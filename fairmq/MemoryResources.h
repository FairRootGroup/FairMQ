/********************************************************************************
 *    Copyright (C) 2018 CERN and copyright holders of ALICE O2 *
 *    Copyright (C) 2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH *
 *                                                                              *
 *              This software is distributed under the terms of the *
 *              GNU Lesser General Public Licence (LGPL) version 3, *
 *                  copied verbatim in the file "LICENSE" *
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
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace fair {
namespace mq {

using byte = unsigned char;

/// All FairMQ related memory resources need to inherit from this interface
/// class for the
/// getMessage() api.
class FairMQMemoryResource : public boost::container::pmr::memory_resource
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

    bool do_is_equal(const boost::container::pmr::memory_resource &other) const noexcept override
    {
        return this == &other;
    };
};

/// This memory resource only watches, does not allocate/deallocate anything.
/// Must be kept alive together with the message as only a pointer to the message is taken.
/// In combination with SpectatorAllocator it allows an stl container to "adopt"
/// the contents of the message buffer as it's own.
class SpectatorMessageResource : public FairMQMemoryResource
{
  public:
    SpectatorMessageResource() = default;

    SpectatorMessageResource(const FairMQMessage *_message)
        : message(_message)
    {
    }

    FairMQMessagePtr getMessage(void * /*p*/) override { return nullptr; }
    FairMQTransportFactory *getTransportFactory() noexcept override { return nullptr; }
    size_t getNumberOfMessages() const noexcept override { return 0; }
    void *setMessage(FairMQMessagePtr) override { return nullptr; }

  protected:
    const FairMQMessage *message;

    void *do_allocate(std::size_t bytes, std::size_t /*alignment*/) override
    {
        if (message) {
            if (bytes > message->GetSize()) {
                throw std::bad_alloc();
            }
            return message->GetData();
        } else {
            return nullptr;
        }
    }

    void do_deallocate(void * /*p*/, std::size_t /*bytes*/, std::size_t /*alignment*/) override
    {
        message = nullptr;
        return;
    }

    bool do_is_equal(const memory_resource &other) const noexcept override
    {
        const SpectatorMessageResource *that =
            dynamic_cast<const SpectatorMessageResource *>(&other);
        if (!that) {
            return false;
        }
        if (that->message == message) {
            return true;
        }
        return false;
    }
};

/// This memory resource only watches, does not allocate/deallocate anything.
/// Ownership of the message is taken. Meant to be used for transparent data
/// adoption in containers.
/// In combination with SpectatorAllocator it allows an stl container to "adopt"
/// the contents of the message buffer as it's own.
class MessageResource : public FairMQMemoryResource
{
  public:
    MessageResource() noexcept = delete;
    MessageResource(const MessageResource &) noexcept = default;
    MessageResource(MessageResource &&) noexcept = default;
    MessageResource &operator=(const MessageResource &) = default;
    MessageResource &operator=(MessageResource &&) = default;

    MessageResource(FairMQMessagePtr message);

    FairMQMessagePtr getMessage(void *p) override { return mUpstream->getMessage(p); }
    void *setMessage(FairMQMessagePtr message) override
    {
        return mUpstream->setMessage(std::move(message));
    }

    FairMQTransportFactory *getTransportFactory() noexcept override { return nullptr; }
    size_t getNumberOfMessages() const noexcept override { return mMessageData ? 1 : 0; }

  protected:
    FairMQMemoryResource *mUpstream{nullptr};
    size_t mMessageSize{0};
    void *mMessageData{nullptr};

    void *do_allocate(std::size_t bytes, std::size_t /*alignment*/) override
    {
        if (bytes > mMessageSize) {
            throw std::bad_alloc();
        }
        return mMessageData;
    }

    void do_deallocate(void * /*p*/, std::size_t /*bytes*/, std::size_t /*alignment*/) override
    {
        getMessage(mMessageData);   // let the message die.
        return;
    }

    bool do_is_equal(const memory_resource & /*other*/) const noexcept override
    {
        // since this uniquely owns the message it can never be equal to anybody
        // else
        return false;
    }
};

/// Special allocator that skips default construction/destruction,
/// allows an stl container to adopt the contents of a buffer as it's own.
/// No ownership of the message or data is taken.
///
/// This in general (as in STL) is a bad idea, but here it is safe to inherit
/// from an allocator since
/// we have no additional data and only override some methods so we don't get
/// into slicing and other
/// problems.
template<typename T>
class SpectatorAllocator : public boost::container::pmr::polymorphic_allocator<T>
{
  public:
    using boost::container::pmr::polymorphic_allocator<T>::polymorphic_allocator;

    // skip default construction of empty elements
    // this is important for two reasons: one: it allows us to adopt an existing
    // buffer (e.g.
    // incoming message) and quickly construct large vectors while skipping the
    // element
    // initialization.
    template<class U>
    void construct(U *)
    {
    }

    // dont try to call destructors, makes no sense since resource is managed
    // externally AND allowed
    // types cannot have side effects
    template<typename U>
    void destroy(U *)
    {
    }

    T *allocate(size_t size)
    {
        return reinterpret_cast<T *>(this->resource()->allocate(size * sizeof(T), 0));
    }

    void deallocate(T *ptr, size_t size)
    {
        this->resource()->deallocate(const_cast<typename std::remove_cv<T>::type *>(ptr), size);
    }
};

/// Special allocator that skips default construction/destruction,
/// allows an stl container to adopt the contents of a buffer as it's own.
/// Ownership of the message is taken.
/// This allocator has a pmr-like interface, but keeps the unique
/// MessageResource as internal state,
/// allowing full resource (associated message) management internally without
/// any global state.
template<typename T>
class OwningMessageSpectatorAllocator
{
  public:
    using value_type = T;

    MessageResource mResource;

    OwningMessageSpectatorAllocator() noexcept = default;
    OwningMessageSpectatorAllocator(const OwningMessageSpectatorAllocator &) noexcept = default;
    OwningMessageSpectatorAllocator(OwningMessageSpectatorAllocator &&) noexcept = default;

    OwningMessageSpectatorAllocator(MessageResource &&resource) noexcept
        : mResource{resource}
    {
    }

    template<class U>
    OwningMessageSpectatorAllocator(const OwningMessageSpectatorAllocator<U> &other) noexcept
        : mResource(other.mResource)
    {
    }

    OwningMessageSpectatorAllocator &operator=(const OwningMessageSpectatorAllocator &other)
    {
        mResource = other.mResource;
        return *this;
    }

    OwningMessageSpectatorAllocator select_on_container_copy_construction() const
    {
        return OwningMessageSpectatorAllocator();
    }

    boost::container::pmr::memory_resource *resource() { return &mResource; }

    // skip default construction of empty elements
    // this is important for two reasons: one: it allows us to adopt an existing
    // buffer (e.g.
    // incoming message) and quickly construct large vectors while skipping the
    // element
    // initialization.
    template<class U>
    void construct(U *)
    {
    }

    // dont try to call destructors, makes no sense since resource is managed
    // externally AND allowed
    // types cannot have side effects
    template<typename U>
    void destroy(U *)
    {
    }

    T *allocate(size_t size)
    {
        return reinterpret_cast<T *>(mResource.allocate(size * sizeof(T), 0));
    }

    void deallocate(T *ptr, size_t size)
    {
        mResource.deallocate(const_cast<typename std::remove_cv<T>::type *>(ptr), size);
    }
};

} /* namespace mq */
} /* namespace fair */

#endif /* FAIR_MQ_MEMORY_RESOURCES_H */
