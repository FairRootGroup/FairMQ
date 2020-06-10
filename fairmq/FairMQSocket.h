/********************************************************************************
 * Copyright (C) 2014-2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIRMQSOCKET_H_
#define FAIRMQSOCKET_H_

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "FairMQMessage.h"
class FairMQTransportFactory;

class FairMQSocket
{
  public:
    FairMQSocket() {}
    FairMQSocket(FairMQTransportFactory* fac) : fTransport(fac) {}

    virtual std::string GetId() const = 0;

    virtual bool Bind(const std::string& address) = 0;
    virtual bool Connect(const std::string& address) = 0;

    virtual int Send(FairMQMessagePtr& msg, int timeout = -1) = 0;
    virtual int Receive(FairMQMessagePtr& msg, int timeout = -1) = 0;
    virtual int64_t Send(std::vector<std::unique_ptr<FairMQMessage>>& msgVec, int timeout = -1) = 0;
    virtual int64_t Receive(std::vector<std::unique_ptr<FairMQMessage>>& msgVec, int timeout = -1) = 0;

    virtual void Close() = 0;

    virtual void SetOption(const std::string& option, const void* value, size_t valueSize) = 0;
    virtual void GetOption(const std::string& option, void* value, size_t* valueSize) = 0;

    virtual void SetLinger(const int value) = 0;
    virtual int GetLinger() const = 0;
    virtual void SetSndBufSize(const int value) = 0;
    virtual int GetSndBufSize() const = 0;
    virtual void SetRcvBufSize(const int value) = 0;
    virtual int GetRcvBufSize() const = 0;
    virtual void SetSndKernelSize(const int value) = 0;
    virtual int GetSndKernelSize() const = 0;
    virtual void SetRcvKernelSize(const int value) = 0;
    virtual int GetRcvKernelSize() const = 0;

    virtual unsigned long GetBytesTx() const = 0;
    virtual unsigned long GetBytesRx() const = 0;
    virtual unsigned long GetMessagesTx() const = 0;
    virtual unsigned long GetMessagesRx() const = 0;

    FairMQTransportFactory* GetTransport() { return fTransport; }
    void SetTransport(FairMQTransportFactory* transport) { fTransport = transport; }

    virtual ~FairMQSocket() {};

  private:
    FairMQTransportFactory* fTransport{nullptr};
};

using FairMQSocketPtr = std::unique_ptr<FairMQSocket>;

namespace fair
{
namespace mq
{

using Socket = FairMQSocket;
using SocketPtr = FairMQSocketPtr;
struct SocketError : std::runtime_error { using std::runtime_error::runtime_error; };

} /* namespace mq */
} /* namespace fair */

#endif /* FAIRMQSOCKET_H_ */
