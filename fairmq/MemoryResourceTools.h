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

//__________________________________________________________________________________________________
/// This memory resource only watches, does not allocate/deallocate anything.
/// Ownership of hte message is taken. Meant to be used for transparent data adoption in containers.
/// In combination with the SpectatorAllocator this is an alternative to using span, as raw memory
/// (e.g. an existing buffer message) will be accessible with appropriate container.
class MessageResource : public FairMQMemoryResource
{

 public:
  MessageResource() noexcept = delete;
  MessageResource(const MessageResource&) noexcept = default;
  MessageResource(MessageResource&&) noexcept = default;
  MessageResource& operator=(const MessageResource&) = default;
  MessageResource& operator=(MessageResource&&) = default;
  MessageResource(FairMQMessagePtr message)
    : mUpstream{ message->GetTransport()->GetMemoryResource() },
      mMessageSize{ message->GetSize() },
      mMessageData{ mUpstream ? mUpstream->setMessage(std::move(message))
                              : throw std::runtime_error("MessageResource::MessageResource upstream is nullptr") }
  {
  }
  FairMQMessagePtr getMessage(void* p) override { return mUpstream->getMessage(p); }
  void* setMessage(FairMQMessagePtr message) override { return mUpstream->setMessage(std::move(message)); }
  FairMQTransportFactory* getTransportFactory() noexcept override { return nullptr; }
  size_t getNumberOfMessages() const noexcept override { return mMessageData ? 1 : 0; }

 protected:
  FairMQMemoryResource* mUpstream{ nullptr };
  size_t mMessageSize{ 0 };
  void* mMessageData{ nullptr };
  bool initialImport{ true };

  void* do_allocate(std::size_t bytes, std::size_t alignment) override
  {
    if (initialImport) {
      if (bytes > mMessageSize) {
        throw std::bad_alloc();
      }
      initialImport = false;
      return mMessageData;
    } else {
      return mUpstream->allocate(bytes, alignment);
    }
  }
  void do_deallocate(void* p, std::size_t bytes, std::size_t alignment) override
  {
    mUpstream->deallocate(p, bytes, alignment);
    return;
  }
  bool do_is_equal(const memory_resource& other) const noexcept override
  {
    return *mUpstream == *dynamic_cast<const FairMQMemoryResource*>(&other);
  }
};

//__________________________________________________________________________________________________
/// This allocator has a pmr-like interface, but keeps the unique MessageResource as internal state,
/// allowing full resource (associated message) management internally without any global state.
template <typename T>
class OwningMessageSpectatorAllocator
{
 public:
  using value_type = T;

  MessageResource mResource;

  OwningMessageSpectatorAllocator() noexcept = default;
  OwningMessageSpectatorAllocator(const OwningMessageSpectatorAllocator&) noexcept = default;
  OwningMessageSpectatorAllocator(OwningMessageSpectatorAllocator&&) noexcept = default;
  OwningMessageSpectatorAllocator(MessageResource&& resource) noexcept : mResource{ resource } {}

  template <class U>
  OwningMessageSpectatorAllocator(const OwningMessageSpectatorAllocator<U>& other) noexcept : mResource(other.mResource)
  {
  }

  OwningMessageSpectatorAllocator& operator=(const OwningMessageSpectatorAllocator& other)
  {
    mResource = other.mResource;
    return *this;
  }

  OwningMessageSpectatorAllocator select_on_container_copy_construction() const
  {
    return OwningMessageSpectatorAllocator();
  }

  boost::container::pmr::memory_resource* resource() { return &mResource; }

  // skip default construction of empty elements
  // this is important for two reasons: one: it allows us to adopt an existing buffer (e.g. incoming message) and
  // quickly construct large vectors while skipping the element initialization.
  template <class U>
  void construct(U*)
  {
  }

  // dont try to call destructors, makes no sense since resource is managed externally AND allowed
  // types cannot have side effects
  template <typename U>
  void destroy(U*)
  {
  }

  T* allocate(size_t size) { return reinterpret_cast<T*>(mResource.allocate(size * sizeof(T), 0)); }
  void deallocate(T* ptr, size_t size)
  {
    mResource.deallocate(const_cast<typename std::remove_cv<T>::type*>(ptr), size);
  }
};

//__________________________________________________________________________________________________
//__________________________________________________________________________________________________
/// Return a std::vector spanned over the contents of the message, takes ownership of the message
template <typename ElemT>
std::vector<ElemT,OwningMessageSpectatorAllocator<ElemT>>
getVector(size_t nelem, FairMQMessagePtr message)
{
  static_assert(std::is_trivially_destructible<ElemT>::value);
  return std::vector<ElemT, OwningMessageSpectatorAllocator<ElemT>>(
    nelem, OwningMessageSpectatorAllocator<ElemT>(MessageResource{ std::move(message) }));
};

} /* namespace mq */
} /* namespace fair */
