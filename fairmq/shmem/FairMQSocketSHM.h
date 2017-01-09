/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
#ifndef FAIRMQSOCKETSHM_H_
#define FAIRMQSOCKETSHM_H_

#include <atomic>

#include <memory> // unique_ptr

#include "FairMQSocket.h"
#include "FairMQContextSHM.h"
#include "FairMQShmManager.h"

class FairMQSocketSHM : public FairMQSocket
{
  public:
    FairMQSocketSHM(const std::string& type, const std::string& name, const int numIoThreads, const std::string& id = "");
    FairMQSocketSHM(const FairMQSocketSHM&) = delete;
    FairMQSocketSHM operator=(const FairMQSocketSHM&) = delete;

    virtual std::string GetId();

    virtual bool Bind(const std::string& address);
    virtual void Connect(const std::string& address);

    virtual int Send(FairMQMessage* msg, const std::string& flag = "");
    virtual int Send(FairMQMessage* msg, const int flags = 0);
    virtual int64_t Send(const std::vector<std::unique_ptr<FairMQMessage>>& msgVec, const int flags = 0);

    virtual int Receive(FairMQMessage* msg, const std::string& flag = "");
    virtual int Receive(FairMQMessage* msg, const int flags = 0);
    virtual int64_t Receive(std::vector<std::unique_ptr<FairMQMessage>>& msgVec, const int flags = 0);

    virtual void* GetSocket() const;
    virtual int GetSocket(int nothing) const;
    virtual void Close();
    virtual void Terminate();

    virtual void Interrupt();
    virtual void Resume();

    virtual void SetOption(const std::string& option, const void* value, size_t valueSize);
    virtual void GetOption(const std::string& option, void* value, size_t* valueSize);

    virtual unsigned long GetBytesTx() const;
    virtual unsigned long GetBytesRx() const;
    virtual unsigned long GetMessagesTx() const;
    virtual unsigned long GetMessagesRx() const;

    virtual bool SetSendTimeout(const int timeout, const std::string& address, const std::string& method);
    virtual int GetSendTimeout() const;
    virtual bool SetReceiveTimeout(const int timeout, const std::string& address, const std::string& method);
    virtual int GetReceiveTimeout() const;

    static int GetConstant(const std::string& constant);

    virtual ~FairMQSocketSHM();

  private:
    void* fSocket;
    std::string fId;
    std::atomic<unsigned long> fBytesTx;
    std::atomic<unsigned long> fBytesRx;
    std::atomic<unsigned long> fMessagesTx;
    std::atomic<unsigned long> fMessagesRx;

    static std::unique_ptr<FairMQContextSHM> fContext;
    static bool fContextInitialized;
};

#endif /* FAIRMQSOCKETSHM_H_ */
