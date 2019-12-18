/********************************************************************************
 * Copyright (C) 2014-2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIRMQTRANSPORTFACTORY_H_
#define FAIRMQTRANSPORTFACTORY_H_

#include <FairMQLogger.h>
#include <FairMQMessage.h>
#include <FairMQPoller.h>
#include <FairMQSocket.h>
#include <FairMQUnmanagedRegion.h>
#include <fairmq/MemoryResources.h>
#include <fairmq/Transports.h>

#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include <stdexcept>
#include <cstddef> // size_t

class FairMQChannel;
namespace fair { namespace mq { class ProgOptions; } }

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

    /// @brief Create empty FairMQMessage
    /// @return pointer to FairMQMessage
    virtual FairMQMessagePtr CreateMessage() = 0;
    /// @brief Create new FairMQMessage of specified size
    /// @param size message size
    /// @return pointer to FairMQMessage
    virtual FairMQMessagePtr CreateMessage(const size_t size) = 0;
    /// @brief Create new FairMQMessage with user provided buffer and size
    /// @param data pointer to user provided buffer
    /// @param size size of the user provided buffer
    /// @param ffn callback, called when the message is transfered (and can be deleted)
    /// @param obj optional helper pointer that can be used in the callback
    /// @return pointer to FairMQMessage
    virtual FairMQMessagePtr CreateMessage(void* data, const size_t size, fairmq_free_fn* ffn, void* hint = nullptr) = 0;

    virtual FairMQMessagePtr CreateMessage(FairMQUnmanagedRegionPtr& unmanagedRegion, void* data, const size_t size, void* hint = 0) = 0;

    /// Create a socket
    virtual FairMQSocketPtr CreateSocket(const std::string& type, const std::string& name) = 0;

    /// Create a poller for a single channel (all subchannels)
    virtual FairMQPollerPtr CreatePoller(const std::vector<FairMQChannel>& channels) const = 0;
    /// Create a poller for specific channels
    virtual FairMQPollerPtr CreatePoller(const std::vector<FairMQChannel*>& channels) const = 0;
    /// Create a poller for specific channels (all subchannels)
    virtual FairMQPollerPtr CreatePoller(const std::unordered_map<std::string, std::vector<FairMQChannel>>& channelsMap, const std::vector<std::string>& channelList) const = 0;

    virtual FairMQUnmanagedRegionPtr CreateUnmanagedRegion(const size_t size, FairMQRegionCallback callback = nullptr, const std::string& path = "", int flags = 0) const = 0;

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

namespace fair
{
namespace mq
{

using TransportFactory = FairMQTransportFactory;
struct TransportFactoryError : std::runtime_error { using std::runtime_error::runtime_error; };

} /* namespace mq */
} /* namespace fair */

#endif /* FAIRMQTRANSPORTFACTORY_H_ */
