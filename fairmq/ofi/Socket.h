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

#include <memory> // unique_ptr
#include <rdma/fabric.h>

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
class Socket : public fair::mq::Socket
{
  public:
    Socket(Context& factory, const std::string& type, const std::string& name, const std::string& id = "");
    Socket(const Socket&) = delete;
    Socket operator=(const Socket&) = delete;

    auto GetId() -> std::string { return fId; }

    auto Bind(const std::string& address) -> bool override;
    auto Connect(const std::string& address) -> void override;

    auto Send(MessagePtr& msg, int timeout = 0) -> int override;
    auto Receive(MessagePtr& msg, int timeout = 0) -> int override;
    auto Send(std::vector<MessagePtr>& msgVec, int timeout = 0) -> int64_t override;
    auto Receive(std::vector<MessagePtr>& msgVec, int timeout = 0) -> int64_t override;

    auto TrySend(MessagePtr& msg) -> int override;
    auto TryReceive(MessagePtr& msg) -> int override;
    auto TrySend(std::vector<MessagePtr>& msgVec) -> int64_t override;
    auto TryReceive(std::vector<MessagePtr>& msgVec) -> int64_t override;

    auto GetSocket() const -> void* override { return fMetaSocket; }
    auto GetSocket(int nothing) const -> int override { return -1; }

    auto Close() -> void override;

    auto SetOption(const std::string& option, const void* value, size_t valueSize) -> void override;
    auto GetOption(const std::string& option, void* value, size_t* valueSize) -> void override;

    auto GetBytesTx() const -> unsigned long override { return fBytesTx; }
    auto GetBytesRx() const -> unsigned long override { return fBytesRx; }
    auto GetMessagesTx() const -> unsigned long override { return fMessagesTx; }
    auto GetMessagesRx() const -> unsigned long override { return fMessagesRx; }

    auto SetSendTimeout(const int timeout, const std::string& address, const std::string& method) -> bool override;
    auto GetSendTimeout() const -> int override;
    auto SetReceiveTimeout(const int timeout, const std::string& address, const std::string& method) -> bool override;
    auto GetReceiveTimeout() const -> int override;

    static auto GetConstant(const std::string& constant) -> int;

    ~Socket() override;

  private:
    void* fMetaSocket;
    fid_ep* fDataEndpoint;
    fid_cq* fDataCompletionQueueTx;
    fid_cq* fDataCompletionQueueRx;
    std::string fId;
    std::atomic<unsigned long> fBytesTx;
    std::atomic<unsigned long> fBytesRx;
    std::atomic<unsigned long> fMessagesTx;
    std::atomic<unsigned long> fMessagesRx;
    Context& fContext;

    int fSndTimeout;
    int fRcvTimeout;

    auto SendImpl(MessagePtr& msg, const int flags, const int timeout) -> int;
    auto ReceiveImpl(MessagePtr& msg, const int flags, const int timeout) -> int;
    auto SendImpl(std::vector<MessagePtr>& msgVec, const int flags, const int timeout) -> int64_t;
    auto ReceiveImpl(std::vector<MessagePtr>& msgVec, const int flags, const int timeout) -> int64_t;
}; /* class Socket */

} /* namespace ofi */
} /* namespace mq */
} /* namespace fair */

#endif /* FAIR_MQ_OFI_SOCKET_H */
