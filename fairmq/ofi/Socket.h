/********************************************************************************
 *    Copyright (C) 2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_OFI_SOCKET_H
#define FAIR_MQ_OFI_SOCKET_H

#include <FairMQSocket.h>
#include <FairMQMessage.h>
#include <fairmq/ofi/Context.h>
#include <fairmq/ofi/ControlMessages.h>

#include <asiofi/connected_endpoint.hpp>
#include <asiofi/memory_resources.hpp>
#include <asiofi/passive_endpoint.hpp>
#include <asiofi/semaphore.hpp>
#include <boost/asio.hpp>
#include <memory> // unique_ptr
#include <mutex>


namespace fair
{
namespace mq
{
namespace ofi
{

/**
 * @class Socket Socket.h <fairmq/ofi/Socket.h>
 * @brief
 *
 * @todo TODO insert long description
 */
class Socket final : public fair::mq::Socket
{
  public:
    Socket(Context& context, const std::string& type, const std::string& name, const std::string& id = "");
    Socket(const Socket&) = delete;
    Socket operator=(const Socket&) = delete;

    auto GetId() const -> std::string override { return fId; }

    auto Bind(const std::string& address) -> bool override;
    auto Connect(const std::string& address) -> bool override;

    auto Send(MessagePtr& msg, int timeout = 0) -> int override;
    auto Receive(MessagePtr& msg, int timeout = 0) -> int override;
    auto Send(std::vector<MessagePtr>& msgVec, int timeout = 0) -> int64_t override;
    auto Receive(std::vector<MessagePtr>& msgVec, int timeout = 0) -> int64_t override;

    auto GetSocket() const -> void* { return nullptr; }

    void SetLinger(const int value) override;
    int GetLinger() const override;
    void SetSndBufSize(const int value) override;
    int GetSndBufSize() const override;
    void SetRcvBufSize(const int value) override;
    int GetRcvBufSize() const override;
    void SetSndKernelSize(const int value) override;
    int GetSndKernelSize() const override;
    void SetRcvKernelSize(const int value) override;
    int GetRcvKernelSize() const override;

    auto Close() -> void override;

    auto SetOption(const std::string& option, const void* value, size_t valueSize) -> void override;
    auto GetOption(const std::string& option, void* value, size_t* valueSize) -> void override;

    auto GetBytesTx() const -> unsigned long override { return fBytesTx; }
    auto GetBytesRx() const -> unsigned long override { return fBytesRx; }
    auto GetMessagesTx() const -> unsigned long override { return fMessagesTx; }
    auto GetMessagesRx() const -> unsigned long override { return fMessagesRx; }

    static auto GetConstant(const std::string& constant) -> int;

    ~Socket() override;

  private:
    Context& fContext;
    asiofi::allocated_pool_resource fControlMemPool;
    std::unique_ptr<asiofi::info> fOfiInfo;
    std::unique_ptr<asiofi::fabric> fOfiFabric;
    std::unique_ptr<asiofi::domain> fOfiDomain;
    std::unique_ptr<asiofi::passive_endpoint> fPassiveEndpoint;
    std::unique_ptr<asiofi::connected_endpoint> fDataEndpoint, fControlEndpoint;
    std::string fId;
    std::atomic<unsigned long> fBytesTx;
    std::atomic<unsigned long> fBytesRx;
    std::atomic<unsigned long> fMessagesTx;
    std::atomic<unsigned long> fMessagesRx;
    Address fRemoteAddr;
    Address fLocalAddr;
    int fSndTimeout;
    int fRcvTimeout;
    std::mutex fSendQueueMutex, fRecvQueueMutex;
    std::queue<std::vector<MessagePtr>> fSendQueue, fRecvQueue;
    std::vector<MessagePtr> fInflightMultiPartMessage;
    int64_t fMultiPartRecvCounter;
    asiofi::synchronized_semaphore fSendPushSem, fSendPopSem, fRecvPushSem, fRecvPopSem;
    std::atomic<bool> fNeedOfiMemoryRegistration;

    auto InitOfi(Address addr) -> void;
    auto BindControlEndpoint() -> void;
    auto BindDataEndpoint() -> void;
    enum class Band { Control, Data };
    auto ConnectEndpoint(std::unique_ptr<asiofi::connected_endpoint>& endpoint, Band type) -> void;
    auto SendQueueReader() -> void;
    auto SendQueueReaderStatic() -> void;
    auto RecvControlQueueReader() -> void;
    auto RecvQueueReaderStatic() -> void;
    auto OnRecvControl(ofi::unique_ptr<ControlMessage> ctrl) -> void;
    auto DataMessageReceived(MessagePtr msg) -> void;
}; /* class Socket */

struct SilentSocketError : SocketError { using SocketError::SocketError; };

} /* namespace ofi */
} /* namespace mq */
} /* namespace fair */

#endif /* FAIR_MQ_OFI_SOCKET_H */
