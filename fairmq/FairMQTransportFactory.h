/********************************************************************************
 * Copyright (C) 2014-2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIRMQTRANSPORTFACTORY_H_
#define FAIRMQTRANSPORTFACTORY_H_

#include <FairMQMessage.h>
#include <FairMQPoller.h>
#include <FairMQSocket.h>
#include <FairMQUnmanagedRegion.h>
#include <fairmq/MemoryResources.h>
#include <fairmq/Transports.h>

#include <string>
#include <memory> // shared_ptr
#include <vector>
#include <unordered_map>
#include <stdexcept>
#include <cstddef> // size_t

class FairMQChannel;
namespace fair::mq { class ProgOptions; }

class FairMQTransportFactory
{
  private:
    /// Topology wide unique id
    const std::string fkId;

    /// The polymorphic memory resource associated with the transport
    fair::mq::ChannelResource fMemoryResource{this};

  public:
    /// ctor
    /// @param id Topology wide unique id, usually the device id.
    FairMQTransportFactory(const std::string& id);

    auto GetId() const -> const std::string { return fkId; };

    /// Get a pointer to the associated polymorphic memory resource
    fair::mq::ChannelResource* GetMemoryResource() { return &fMemoryResource; }
    operator fair::mq::ChannelResource*() { return &fMemoryResource; }

    /// @brief Create empty FairMQMessage (for receiving)
    /// @return pointer to FairMQMessage
    virtual FairMQMessagePtr CreateMessage() = 0;
    /// @brief Create empty FairMQMessage (for receiving), align received buffer to specified alignment
    /// @param alignment alignment to align received buffer to
    /// @return pointer to FairMQMessage
    virtual FairMQMessagePtr CreateMessage(fair::mq::Alignment alignment) = 0;
    /// @brief Create new FairMQMessage of specified size
    /// @param size message size
    /// @return pointer to FairMQMessage
    virtual FairMQMessagePtr CreateMessage(const size_t size) = 0;
    /// @brief Create new FairMQMessage of specified size and alignment
    /// @param size message size
    /// @param alignment message alignment
    /// @return pointer to FairMQMessage
    virtual FairMQMessagePtr CreateMessage(const size_t size, fair::mq::Alignment alignment) = 0;
    /// @brief Create new FairMQMessage with user provided buffer and size
    /// @param data pointer to user provided buffer
    /// @param size size of the user provided buffer
    /// @param ffn callback, called when the message is transfered (and can be deleted)
    /// @param obj optional helper pointer that can be used in the callback
    /// @return pointer to FairMQMessage
    virtual FairMQMessagePtr CreateMessage(void* data, const size_t size, fairmq_free_fn* ffn, void* hint = nullptr) = 0;
    /// @brief create a message with the buffer located within the corresponding unmanaged region
    /// @param unmanagedRegion the unmanaged region that this message buffer belongs to
    /// @param data message buffer (must be within the region - checked at runtime by the transport)
    /// @param size size of the message
    /// @param hint optional parameter, returned to the user in the FairMQRegionCallback
    virtual FairMQMessagePtr CreateMessage(FairMQUnmanagedRegionPtr& unmanagedRegion, void* data, const size_t size, void* hint = 0) = 0;

    /// @brief Create a socket
    virtual FairMQSocketPtr CreateSocket(const std::string& type, const std::string& name) = 0;

    /// @brief Create a poller for a single channel (all subchannels)
    virtual FairMQPollerPtr CreatePoller(const std::vector<FairMQChannel>& channels) const = 0;
    /// @brief Create a poller for specific channels
    virtual FairMQPollerPtr CreatePoller(const std::vector<FairMQChannel*>& channels) const = 0;
    /// @brief Create a poller for specific channels (all subchannels)
    virtual FairMQPollerPtr CreatePoller(const std::unordered_map<std::string, std::vector<FairMQChannel>>& channelsMap, const std::vector<std::string>& channelList) const = 0;

    /// @brief Create new UnmanagedRegion
    /// @param size size of the region
    /// @param callback callback to be called when a message belonging to this region is no longer needed by the transport
    /// @param path optional parameter to pass to the underlying transport
    /// @param flags optional parameter to pass to the underlying transport
    /// @return pointer to UnmanagedRegion
    virtual FairMQUnmanagedRegionPtr CreateUnmanagedRegion(const size_t size, FairMQRegionCallback callback = nullptr, const std::string& path = "", int flags = 0, fair::mq::RegionConfig cfg = fair::mq::RegionConfig()) = 0;
    virtual FairMQUnmanagedRegionPtr CreateUnmanagedRegion(const size_t size, FairMQRegionBulkCallback callback = nullptr, const std::string& path = "", int flags = 0, fair::mq::RegionConfig cfg = fair::mq::RegionConfig()) = 0;
    /// @brief Create new UnmanagedRegion
    /// @param size size of the region
    /// @param userFlags flags to be stored with the region, have no effect on the transport, but can be retrieved from the region by the user
    /// @param callback callback to be called when a message belonging to this region is no longer needed by the transport
    /// @param path optional parameter to pass to the underlying transport
    /// @param flags optional parameter to pass to the underlying transport
    /// @return pointer to UnmanagedRegion
    virtual FairMQUnmanagedRegionPtr CreateUnmanagedRegion(const size_t size, const int64_t userFlags, FairMQRegionCallback callback = nullptr, const std::string& path = "", int flags = 0, fair::mq::RegionConfig cfg = fair::mq::RegionConfig()) = 0;
    virtual FairMQUnmanagedRegionPtr CreateUnmanagedRegion(const size_t size, const int64_t userFlags, FairMQRegionBulkCallback callback = nullptr, const std::string& path = "", int flags = 0, fair::mq::RegionConfig cfg = fair::mq::RegionConfig()) = 0;

    /// @brief Subscribe to region events (creation, destruction, ...)
    /// @param callback the callback that is called when a region event occurs
    virtual void SubscribeToRegionEvents(FairMQRegionEventCallback callback) = 0;
    /// @brief Check if there is an active subscription to region events
    /// @return true/false
    virtual bool SubscribedToRegionEvents() = 0;
    /// @brief Unsubscribe from region events
    virtual void UnsubscribeFromRegionEvents() = 0;

    virtual std::vector<FairMQRegionInfo> GetRegionInfo() = 0;

    /// Get transport type
    virtual fair::mq::Transport GetType() const = 0;

    virtual void Interrupt() = 0;
    virtual void Resume() = 0;
    virtual void Reset() = 0;

    virtual ~FairMQTransportFactory() {};

    static auto CreateTransportFactory(const std::string& type, const std::string& id = "", const fair::mq::ProgOptions* config = nullptr) -> std::shared_ptr<FairMQTransportFactory>;

    static void FairMQNoCleanup(void* /*data*/, void* /*obj*/)
    {
    }

    template<typename T>
    static void FairMQSimpleMsgCleanup(void* /*data*/, void* obj)
    {
        delete static_cast<T*>(obj);
    }

    template<typename T>
    FairMQMessagePtr NewSimpleMessage(const T& data)
    {
        // todo: is_trivially_copyable not available on gcc < 5, workaround?
        // static_assert(std::is_trivially_copyable<T>::value, "The argument type for NewSimpleMessage has to be trivially copyable!");
        T* dataCopy = new T(data);
        return CreateMessage(dataCopy, sizeof(T), FairMQSimpleMsgCleanup<T>, dataCopy);
    }

    template<std::size_t N>
    FairMQMessagePtr NewSimpleMessage(const char(&data)[N])
    {
        std::string* msgStr = new std::string(data);
        return CreateMessage(const_cast<char*>(msgStr->c_str()), msgStr->length(), FairMQSimpleMsgCleanup<std::string>, msgStr);
    }

    FairMQMessagePtr NewSimpleMessage(const std::string& str)
    {

        std::string* msgStr = new std::string(str);
        return CreateMessage(const_cast<char*>(msgStr->c_str()), msgStr->length(), FairMQSimpleMsgCleanup<std::string>, msgStr);
    }

    template<typename T>
    FairMQMessagePtr NewStaticMessage(const T& data)
    {
        return CreateMessage(data, sizeof(T), FairMQNoCleanup, nullptr);
    }

    FairMQMessagePtr NewStaticMessage(const std::string& str)
    {
        return CreateMessage(const_cast<char*>(str.c_str()), str.length(), FairMQNoCleanup, nullptr);
    }
};

namespace fair::mq
{

using TransportFactory = FairMQTransportFactory;
struct TransportFactoryError : std::runtime_error { using std::runtime_error::runtime_error; };

} // namespace fair::mq

#endif /* FAIRMQTRANSPORTFACTORY_H_ */
