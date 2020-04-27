/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
#ifndef FAIR_MQ_SHMEM_SOCKET_H_
#define FAIR_MQ_SHMEM_SOCKET_H_

#include "Manager.h"

#include <FairMQSocket.h>
#include <FairMQMessage.h>

#include <atomic>
#include <memory> // unique_ptr

class FairMQTransportFactory;

namespace fair
{
namespace mq
{
namespace shmem
{

class Socket final : public fair::mq::Socket
{
  public:
    Socket(Manager& manager, const std::string& type, const std::string& name, const std::string& id = "", void* context = nullptr, FairMQTransportFactory* fac = nullptr);
    Socket(const Socket&) = delete;
    Socket operator=(const Socket&) = delete;

    std::string GetId() const override { return fId; }

    bool Bind(const std::string& address) override;
    bool Connect(const std::string& address) override;

    int Send(MessagePtr& msg, const int timeout = -1) override;
    int Receive(MessagePtr& msg, const int timeout = -1) override;
    int64_t Send(std::vector<MessagePtr>& msgVec, const int timeout = -1) override;
    int64_t Receive(std::vector<MessagePtr>& msgVec, const int timeout = -1) override;

    void* GetSocket() const { return fSocket; }

    void Close() override;

    void SetOption(const std::string& option, const void* value, size_t valueSize) override;
    void GetOption(const std::string& option, void* value, size_t* valueSize) override;

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

    unsigned long GetBytesTx() const override { return fBytesTx; }
    unsigned long GetBytesRx() const override { return fBytesRx; }
    unsigned long GetMessagesTx() const override { return fMessagesTx; }
    unsigned long GetMessagesRx() const override { return fMessagesRx; }

    static int GetConstant(const std::string& constant);

    ~Socket() override { Close(); }

  private:
    void* fSocket;
    Manager& fManager;
    std::string fId;
    std::atomic<unsigned long> fBytesTx;
    std::atomic<unsigned long> fBytesRx;
    std::atomic<unsigned long> fMessagesTx;
    std::atomic<unsigned long> fMessagesRx;

    int fSndTimeout;
    int fRcvTimeout;
};

}
}
}

#endif /* FAIR_MQ_SHMEM_SOCKET_H_ */
