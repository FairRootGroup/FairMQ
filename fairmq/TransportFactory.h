/********************************************************************************
 * Copyright (C) 2021-2023 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_TRANSPORTFACTORY_H
#define FAIR_MQ_TRANSPORTFACTORY_H

#include <cstddef>   // size_t
#include <fairmq/MemoryResources.h>
#include <fairmq/Message.h>
#include <fairmq/Poller.h>
#include <fairmq/Socket.h>
#include <fairmq/Transports.h>
#include <fairmq/UnmanagedRegion.h>
#include <memory>   // shared_ptr
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace fair::mq {

class Channel;
class ProgOptions;

class TransportFactory
{
  private:
    /// Topology wide unique id
    const std::string fkId;

    /// The polymorphic memory resource associated with the transport
    ChannelResource fMemoryResource{this};

  public:
    /// ctor
    /// @param id Topology wide unique id, usually the device id.
    TransportFactory(std::string id)
        : fkId(std::move(id))
    {}

    TransportFactory(const TransportFactory&) = delete;
    TransportFactory(TransportFactory&&) = delete;
    TransportFactory& operator=(const TransportFactory&) = delete;
    TransportFactory& operator=(TransportFactory&&) = delete;

    auto GetId() const -> const std::string { return fkId; };

    /// Get a pointer to the associated polymorphic memory resource
    ChannelResource* GetMemoryResource() { return &fMemoryResource; }
    operator ChannelResource*() { return &fMemoryResource; }

    /// @brief Create empty Message (for receiving)
    /// @return pointer to Message
    virtual MessagePtr CreateMessage() = 0;
    /// @brief Create empty Message (for receiving), align received buffer to specified alignment
    /// @param alignment alignment to align received buffer to
    /// @return pointer to Message
    virtual MessagePtr CreateMessage(Alignment alignment) = 0;
    /// @brief Create new Message of specified size
    /// @param size message size
    /// @return pointer to Message
    virtual MessagePtr CreateMessage(size_t size) = 0;
    /// @brief Create new Message of specified size and alignment
    /// @param size message size
    /// @param alignment message alignment
    /// @return pointer to Message
    virtual MessagePtr CreateMessage(size_t size, Alignment alignment) = 0;
    /// @brief Create new Message with user provided buffer and size
    /// @param data pointer to user provided buffer
    /// @param size size of the user provided buffer
    /// @param ffn callback, called when the message is transfered (and can be deleted)
    /// @param obj optional helper pointer that can be used in the callback
    /// @return pointer to Message
    virtual MessagePtr CreateMessage(void* data,
                                     size_t size,
                                     FreeFn* ffn,
                                     void* hint = nullptr) = 0;
    /// @brief create a message with the buffer located within the corresponding unmanaged region
    /// @param unmanagedRegion the unmanaged region that this message buffer belongs to
    /// @param data message buffer (must be within the region - checked at runtime by the transport)
    /// @param size size of the message
    /// @param hint optional parameter, returned to the user in the RegionCallback
    virtual MessagePtr CreateMessage(UnmanagedRegionPtr& unmanagedRegion,
                                     void* data,
                                     size_t size,
                                     void* hint = nullptr) = 0;

    /// @brief Create a socket
    virtual SocketPtr CreateSocket(const std::string& type, const std::string& name) = 0;

    /// @brief Create a poller for a single channel (all subchannels)
    virtual PollerPtr CreatePoller(const std::vector<Channel>& channels) const = 0;
    /// @brief Create a poller for specific channels
    virtual PollerPtr CreatePoller(const std::vector<Channel*>& channels) const = 0;
    /// @brief Create a poller for specific channels (all subchannels)
    virtual PollerPtr CreatePoller(
        const std::unordered_map<std::string, std::vector<Channel>>& channelsMap,
        const std::vector<std::string>& channelList) const = 0;

    /// @brief Create new UnmanagedRegion
    /// @param size size of the region
    /// @param callback callback to be called when a message belonging to this region is no longer
    /// needed by the transport
    /// @param path optional parameter to pass to the underlying transport
    /// @param flags optional parameter to pass to the underlying transport
    /// @return pointer to UnmanagedRegion
    // [[deprecated("Use CreateUnmanagedRegion(size_t size, RegionCallback callback, RegionConfig cfg)")]]
    virtual UnmanagedRegionPtr CreateUnmanagedRegion(size_t size,
                                                     RegionCallback callback = nullptr,
                                                     const std::string& path = "",
                                                     int flags = 0,
                                                     RegionConfig cfg = RegionConfig()) = 0;
    // [[deprecated("Use CreateUnmanagedRegion(size_t size, RegionCallback callback, RegionConfig cfg)")]]
    virtual UnmanagedRegionPtr CreateUnmanagedRegion(size_t size,
                                                     RegionBulkCallback bulkCallback = nullptr,
                                                     const std::string& path = "",
                                                     int flags = 0,
                                                     RegionConfig cfg = RegionConfig()) = 0;
    /// @brief Create new UnmanagedRegion
    /// @param size size of the region
    /// @param userFlags flags to be stored with the region, have no effect on the transport, but
    /// can be retrieved from the region by the user
    /// @param callback callback to be called when a message belonging to this region is no longer
    /// needed by the transport
    /// @param path optional parameter to pass to the underlying transport
    /// @param flags optional parameter to pass to the underlying transport
    /// @return pointer to UnmanagedRegion
    // [[deprecated("Use CreateUnmanagedRegion(size_t size, RegionCallback callback, RegionConfig cfg)")]]
    virtual UnmanagedRegionPtr CreateUnmanagedRegion(size_t size,
                                                     int64_t userFlags,
                                                     RegionCallback callback = nullptr,
                                                     const std::string& path = "",
                                                     int flags = 0,
                                                     RegionConfig cfg = RegionConfig()) = 0;
    // [[deprecated("Use CreateUnmanagedRegion(size_t size, RegionCallback callback, RegionConfig cfg)")]]
    virtual UnmanagedRegionPtr CreateUnmanagedRegion(size_t size,
                                                     int64_t userFlags,
                                                     RegionBulkCallback bulkCallback = nullptr,
                                                     const std::string& path = "",
                                                     int flags = 0,
                                                     RegionConfig cfg = RegionConfig()) = 0;

    /// @brief Create new UnmanagedRegion
    /// @param size size of the region
    /// @param callback callback to be called when a message belonging to this region is no longer needed by the transport
    /// @param cfg region configuration
    /// @return pointer to UnmanagedRegion
    virtual UnmanagedRegionPtr CreateUnmanagedRegion(size_t size, RegionCallback callback, RegionConfig cfg) = 0;

    /// @brief Create new UnmanagedRegion
    /// @param size size of the region
    /// @param bulkCallback callback to be called when message(s) belonging to this region is no longer needed by the transport
    /// @param cfg region configuration
    /// @return pointer to UnmanagedRegion
    virtual UnmanagedRegionPtr CreateUnmanagedRegion(size_t size, RegionBulkCallback bulkCallback, RegionConfig cfg) = 0;

    /// @brief Subscribe to region events (creation, destruction, ...)
    /// @param callback the callback that is called when a region event occurs
    virtual void SubscribeToRegionEvents(RegionEventCallback callback) = 0;
    /// @brief Check if there is an active subscription to region events
    /// @return true/false
    virtual bool SubscribedToRegionEvents() = 0;
    /// @brief Unsubscribe from region events
    virtual void UnsubscribeFromRegionEvents() = 0;

    virtual std::vector<RegionInfo> GetRegionInfo() = 0;

    /// Get transport type
    virtual Transport GetType() const = 0;

    virtual void Interrupt() = 0;
    virtual void Resume() = 0;
    virtual void Reset() = 0;

    virtual ~TransportFactory() = default;

    static auto CreateTransportFactory(const std::string& type,
                                       const std::string& id = "",
                                       const ProgOptions* config = nullptr)
        -> std::shared_ptr<TransportFactory>;

    static void NoCleanup(void* /*data*/, void* /*obj*/) {}

    template<typename T>
    static void SimpleMsgCleanup(void* /*data*/, void* obj)
    {
        delete static_cast<T*>(obj);
    }

    template<typename T>
    MessagePtr NewSimpleMessage(const T& data)
    {
        // todo: is_trivially_copyable not available on gcc < 5, workaround?
        // static_assert(std::is_trivially_copyable<T>::value, "The argument type for
        // NewSimpleMessage has to be trivially copyable!");
        T* dataCopy = new T(data);
        return CreateMessage(dataCopy, sizeof(T), SimpleMsgCleanup<T>, dataCopy);
    }

    template<std::size_t N>
    MessagePtr NewSimpleMessage(const char (&data)[N])
    {
        std::string* msgStr = new std::string(data);
        return CreateMessage(const_cast<char*>(msgStr->c_str()),
                             msgStr->length(),
                             SimpleMsgCleanup<std::string>,
                             msgStr);
    }

    MessagePtr NewSimpleMessage(const std::string& str)
    {

        std::string* msgStr = new std::string(str);
        return CreateMessage(const_cast<char*>(msgStr->c_str()),
                             msgStr->length(),
                             SimpleMsgCleanup<std::string>,
                             msgStr);
    }

    template<typename T>
    MessagePtr NewStaticMessage(const T& data)
    {
        return CreateMessage(data, sizeof(T), NoCleanup, nullptr);
    }

    MessagePtr NewStaticMessage(const std::string& str)
    {
        return CreateMessage(const_cast<char*>(str.c_str()), str.length(), NoCleanup, nullptr);
    }
};

struct TransportFactoryError : std::runtime_error
{
    using std::runtime_error::runtime_error;
};

}   // namespace fair::mq

#endif   // FAIR_MQ_TRANSPORTFACTORY_H
