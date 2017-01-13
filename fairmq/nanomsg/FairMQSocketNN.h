/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQSocketNN.h
 *
 * @since 2013-12-05
 * @author A. Rybalchenko
 */

#ifndef FAIRMQSOCKETNN_H_
#define FAIRMQSOCKETNN_H_

#include <vector>
#include <atomic>

#include "FairMQSocket.h"
#include "FairMQMessage.h"

class FairMQSocketNN : public FairMQSocket
{
  public:
    FairMQSocketNN(const std::string& type, const std::string& name, const int numIoThreads, const std::string& id = ""); // numIoThreads is not used in nanomsg.
    FairMQSocketNN(const FairMQSocketNN&) = delete;
    FairMQSocketNN operator=(const FairMQSocketNN&) = delete;

    virtual std::string GetId();

    virtual bool Bind(const std::string& address);
    virtual void Connect(const std::string& address);

    virtual int Send(FairMQMessagePtr& msg, const int flags = 0);
    virtual int Receive(FairMQMessagePtr& msg, const int flags = 0);

    virtual int64_t Send(std::vector<std::unique_ptr<FairMQMessage>>& msgVec, const int flags = 0);
    virtual int64_t Receive(std::vector<std::unique_ptr<FairMQMessage>>& msgVec, const int flags = 0);

    virtual void* GetSocket() const;
    virtual int GetSocket(int nothing) const;
    virtual void Close();
    virtual void Terminate();

    virtual void Interrupt();
    virtual void Resume();

    virtual void SetOption(const std::string& option, const void* value, size_t valueSize);
    virtual void GetOption(const std::string& option, void* value, size_t* valueSize);

    unsigned long GetBytesTx() const;
    unsigned long GetBytesRx() const;
    unsigned long GetMessagesTx() const;
    unsigned long GetMessagesRx() const;

    virtual bool SetSendTimeout(const int timeout, const std::string& address, const std::string& method);
    virtual int GetSendTimeout() const;
    virtual bool SetReceiveTimeout(const int timeout, const std::string& address, const std::string& method);
    virtual int GetReceiveTimeout() const;

    static int GetConstant(const std::string& constant);

    virtual ~FairMQSocketNN();

  private:
    int fSocket;
    std::string fId;
    std::atomic<unsigned long> fBytesTx;
    std::atomic<unsigned long> fBytesRx;
    std::atomic<unsigned long> fMessagesTx;
    std::atomic<unsigned long> fMessagesRx;
    static std::atomic<bool> fInterrupted;
};

#endif /* FAIRMQSOCKETNN_H_ */
