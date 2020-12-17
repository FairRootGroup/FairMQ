/********************************************************************************
 * Copyright (C) 2014-2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIRMQSOCKET_H_
#define FAIRMQSOCKET_H_

#include "FairMQMessage.h"
#include <fairmq/Buffer.h>
#include <fairmq/Msg.h>

#include <memory>
#include <optional>
#include <ostream>
#include <stdexcept>
#include <string>
#include <vector>

class FairMQTransportFactory;

namespace fair
{
namespace mq
{

enum class TransferCode : int
{
    success = 0,
    error = -1,
    timeout = -2,
    interrupted = -3
};

struct TransferResult
{
    size_t nbytes;
    TransferCode code;
    std::optional<Msg> msg;
};

} // namespace mq
} // namespace fair

class FairMQSocket
{
  public:
    FairMQSocket() {}
    FairMQSocket(FairMQTransportFactory* fac) : fTransport(fac) {}

    virtual std::string GetId() const = 0;

    virtual bool Bind(const std::string& address) = 0;
    virtual bool Connect(const std::string& address) = 0;

    virtual int64_t Send(FairMQMessagePtr& msg, int timeout = -1) = 0;
    virtual int64_t Receive(FairMQMessagePtr& msg, int timeout = -1) = 0;
    virtual int64_t Send(std::vector<FairMQMessagePtr>& msgVec, int timeout = -1) = 0;
    virtual int64_t Receive(std::vector<FairMQMessagePtr>& msgVec, int timeout = -1) = 0;

    virtual fair::mq::TransferResult Send(fair::mq::Buffer buf, int timeout = -1) = 0;
    virtual fair::mq::TransferResult Send(fair::mq::Msg msg, int timeout = -1) = 0;
    virtual fair::mq::TransferResult Receive(int timeout = -1) = 0;

    virtual void Close() = 0;

    virtual void SetOption(const std::string& option, const void* value, size_t valueSize) = 0;
    virtual void GetOption(const std::string& option, void* value, size_t* valueSize) = 0;

    /// If the backend supports it, fills the unsigned integer @a events with the ZMQ_EVENTS value
    /// DISCLAIMER: this API is experimental and unsupported and might be dropped / refactored in
    /// the future.
    virtual void Events(uint32_t* events) = 0;
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
