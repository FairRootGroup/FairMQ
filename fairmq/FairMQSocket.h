/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQSocket.h
 *
 * @since 2012-12-05
 * @author D. Klein, A. Rybalchenko
 */

#ifndef FAIRMQSOCKET_H_
#define FAIRMQSOCKET_H_

#include <string>
#include <vector>
#include <memory>

#include "FairMQMessage.h"

class FairMQSocket
{
  public:
    const int SNDMORE;
    const int RCVMORE;
    const int NOBLOCK;

    FairMQSocket(int sndMore, int rcvMore, int noBlock)
        : SNDMORE(sndMore)
        , RCVMORE(rcvMore)
        , NOBLOCK(noBlock)
        {}

    virtual std::string GetId() = 0;

    virtual bool Bind(const std::string& address) = 0;
    virtual void Connect(const std::string& address) = 0;
    virtual bool Attach(const std::string& address, bool serverish = false);

    virtual int Send(FairMQMessagePtr& msg, const int flags = 0) = 0;
    virtual int Receive(FairMQMessagePtr& msg, const int flags = 0) = 0;

    virtual int64_t Send(std::vector<std::unique_ptr<FairMQMessage>>& msgVec, const int flags = 0) = 0;
    virtual int64_t Receive(std::vector<std::unique_ptr<FairMQMessage>>& msgVec, const int flags = 0) = 0;

    virtual void* GetSocket() const = 0;
    virtual int GetSocket(int nothing) const = 0;
    virtual void Close() = 0;

    virtual void Interrupt() = 0;
    virtual void Resume() = 0;

    virtual void SetOption(const std::string& option, const void* value, size_t valueSize) = 0;
    virtual void GetOption(const std::string& option, void* value, size_t* valueSize) = 0;

    virtual unsigned long GetBytesTx() const = 0;
    virtual unsigned long GetBytesRx() const = 0;
    virtual unsigned long GetMessagesTx() const = 0;
    virtual unsigned long GetMessagesRx() const = 0;

    virtual bool SetSendTimeout(const int timeout, const std::string& address, const std::string& method) = 0;
    virtual int GetSendTimeout() const = 0;
    virtual bool SetReceiveTimeout(const int timeout, const std::string& address, const std::string& method) = 0;
    virtual int GetReceiveTimeout() const = 0;

    virtual ~FairMQSocket() {};
};

using FairMQSocketPtr = std::unique_ptr<FairMQSocket>;

#endif /* FAIRMQSOCKET_H_ */
