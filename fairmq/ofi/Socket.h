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

#include <boost/asio.hpp>
#include <memory> // unique_ptr
#include <netinet/in.h>
#include <rdma/fabric.h>
class FairMQTransportFactory;

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
    Socket(Context& factory, const std::string& type, const std::string& name, const std::string& id = "", FairMQTransportFactory* fac);
    Socket(const Socket&) = delete;
    Socket operator=(const Socket&) = delete;

    auto GetId() -> std::string { return fId; }

    auto Bind(const std::string& address) -> bool override;
    auto Connect(const std::string& address) -> bool override;

    auto Send(MessagePtr& msg, int timeout = 0) -> int override;
    auto Receive(MessagePtr& msg, int timeout = 0) -> int override;
    auto Send(std::vector<MessagePtr>& msgVec, int timeout = 0) -> int64_t override;
    auto Receive(std::vector<MessagePtr>& msgVec, int timeout = 0) -> int64_t override;

    auto TrySend(MessagePtr& msg) -> int override;
    auto TryReceive(MessagePtr& msg) -> int override;
    auto TrySend(std::vector<MessagePtr>& msgVec) -> int64_t override;
    auto TryReceive(std::vector<MessagePtr>& msgVec) -> int64_t override;

    auto GetSocket() const -> void* { return fControlSocket; }

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
    void* fControlSocket;
    void* fMonitorSocket;
    fid_ep* fDataEndpoint;
    fid_cq* fDataCompletionQueueTx;
    fid_cq* fDataCompletionQueueRx;
    std::string fId;
    std::atomic<unsigned long> fBytesTx;
    std::atomic<unsigned long> fBytesRx;
    std::atomic<unsigned long> fMessagesTx;
    std::atomic<unsigned long> fMessagesRx;
    Context& fContext;
    fi_addr_t fRemoteDataAddr;
    sockaddr_in fLocalDataAddr;
    bool fWaitingForControlPeer;
    boost::asio::io_service::strand fIoStrand;

    int fSndTimeout;
    int fRcvTimeout;

    auto SendImpl(MessagePtr& msg, const int flags, const int timeout) -> int;
    auto ReceiveImpl(MessagePtr& msg, const int flags, const int timeout) -> int;
    auto SendImpl(std::vector<MessagePtr>& msgVec, const int flags, const int timeout) -> int64_t;
    auto ReceiveImpl(std::vector<MessagePtr>& msgVec, const int flags, const int timeout) -> int64_t;

    auto InitDataEndpoint() -> void;
    auto WaitForControlPeer() -> void;
    auto AnnounceDataAddress() -> void;
    // auto SendControlMessage(std::unique_ptr<ControlMessage> ctrl) -> void;
    // auto ReceiveControlMessage() -> std::unique_ptr<ControlMessage>;
    // auto ProcessDataAddressAnnouncement(std::unique_ptr<ControlMessage> ctrl) -> void;
    auto ConnectControlSocket(Context::Address address) -> void;
    auto BindControlSocket(Context::Address address) -> void;
}; /* class Socket */

// helper function to clean up the object holding the data after it is transported.
void free_string(void* /*data*/, void* hint);

struct SilentSocketError : SocketError { using SocketError::SocketError; };

} /* namespace ofi */
} /* namespace mq */
} /* namespace fair */

#endif /* FAIR_MQ_OFI_SOCKET_H */
