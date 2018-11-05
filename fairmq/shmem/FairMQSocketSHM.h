/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
#ifndef FAIRMQSOCKETSHM_H_
#define FAIRMQSOCKETSHM_H_

#include "FairMQSocket.h"
#include "FairMQMessage.h"

#include <fairmq/shmem/Manager.h>

#include <atomic>
#include <memory> // unique_ptr

class FairMQSocketSHM final : public FairMQSocket
{
  public:
    FairMQSocketSHM(fair::mq::shmem::Manager& manager, const std::string& type, const std::string& name, const std::string& id = "", void* context = nullptr);
    FairMQSocketSHM(const FairMQSocketSHM&) = delete;
    FairMQSocketSHM operator=(const FairMQSocketSHM&) = delete;

    std::string GetId() override { return fId; }

    bool Bind(const std::string& address) override;
    bool Connect(const std::string& address) override;

    int Send(FairMQMessagePtr& msg, const int timeout = -1) override;
    int Receive(FairMQMessagePtr& msg, const int timeout = -1) override;
    int64_t Send(std::vector<std::unique_ptr<FairMQMessage>>& msgVec, const int timeout = -1) override;
    int64_t Receive(std::vector<std::unique_ptr<FairMQMessage>>& msgVec, const int timeout = -1) override;

    void* GetSocket() const;

    void Close() override;

    static void Interrupt();
    static void Resume();

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

    unsigned long GetBytesTx() const override;
    unsigned long GetBytesRx() const override;
    unsigned long GetMessagesTx() const override;
    unsigned long GetMessagesRx() const override;

    static int GetConstant(const std::string& constant);

    ~FairMQSocketSHM() override;

  private:
    void* fSocket;
    fair::mq::shmem::Manager& fManager;
    std::string fId;
    std::atomic<unsigned long> fBytesTx;
    std::atomic<unsigned long> fBytesRx;
    std::atomic<unsigned long> fMessagesTx;
    std::atomic<unsigned long> fMessagesRx;

    static std::atomic<bool> fInterrupted;

    int fSndTimeout;
    int fRcvTimeout;
};

#endif /* FAIRMQSOCKETSHM_H_ */
