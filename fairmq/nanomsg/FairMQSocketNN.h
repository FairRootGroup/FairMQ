/********************************************************************************
 * Copyright (C) 2014-2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIRMQSOCKETNN_H_
#define FAIRMQSOCKETNN_H_

#include <vector>
#include <atomic>

#include "FairMQSocket.h"
#include "FairMQMessage.h"
class FairMQTransportFactory;

class FairMQSocketNN final : public FairMQSocket
{
  public:
    FairMQSocketNN(const std::string& type, const std::string& name, const std::string& id = "", FairMQTransportFactory* fac = nullptr);
    FairMQSocketNN(const FairMQSocketNN&) = delete;
    FairMQSocketNN operator=(const FairMQSocketNN&) = delete;

    std::string GetId() const override { return fId; }

    bool Bind(const std::string& address) override;
    bool Connect(const std::string& address) override;

    int Send(FairMQMessagePtr& msg, const int timeout = -1) override;
    int Receive(FairMQMessagePtr& msg, const int timeout = -1) override;
    int64_t Send(std::vector<std::unique_ptr<FairMQMessage>>& msgVec, const int timeout = -1) override;
    int64_t Receive(std::vector<std::unique_ptr<FairMQMessage>>& msgVec, const int timeout = -1) override;

    int GetSocket() const;

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

    ~FairMQSocketNN() override;

  private:
    int fSocket;
    std::string fId;
    std::atomic<unsigned long> fBytesTx;
    std::atomic<unsigned long> fBytesRx;
    std::atomic<unsigned long> fMessagesTx;
    std::atomic<unsigned long> fMessagesRx;

    static std::atomic<bool> fInterrupted;

    int fSndTimeout;
    int fRcvTimeout;
    int fLinger;
};

#endif /* FAIRMQSOCKETNN_H_ */
