/********************************************************************************
 *    Copyright (C) 2021 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_SOCKET_H
#define FAIR_MQ_SOCKET_H

#include <fairmq/Message.h>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace fair::mq {

class TransportFactory;

enum class TransferCode : int
{
    success = 0,
    error = -1,
    timeout = -2,
    interrupted = -3
};

struct Socket
{
    Socket() = default;
    Socket(TransportFactory* fac)
        : fTransport(fac)
    {}

    virtual std::string GetId() const = 0;

    virtual bool Bind(const std::string& address) = 0;
    virtual bool Connect(const std::string& address) = 0;

    virtual int64_t Send(MessagePtr& msg, int timeout = -1) = 0;
    virtual int64_t Receive(MessagePtr& msg, int timeout = -1) = 0;
    virtual int64_t Send(std::vector<std::unique_ptr<Message>>& msgVec, int timeout = -1) = 0;
    virtual int64_t Receive(std::vector<std::unique_ptr<Message>>& msgVec, int timeout = -1) = 0;

    virtual void Close() = 0;

    virtual void SetOption(const std::string& option, const void* value, size_t valueSize) = 0;
    virtual void GetOption(const std::string& option, void* value, size_t* valueSize) = 0;

    /// If the backend supports it, fills the unsigned integer @a events with the ZMQ_EVENTS value
    /// DISCLAIMER: this API is experimental and unsupported and might be dropped / refactored in
    /// the future.
    virtual int Events(uint32_t* events) = 0;
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

    TransportFactory* GetTransport() { return fTransport; }
    void SetTransport(TransportFactory* transport) { fTransport = transport; }

    virtual ~Socket() = default;

  private:
    TransportFactory* fTransport{nullptr};
};

using SocketPtr = std::unique_ptr<Socket>;

struct SocketError : std::runtime_error
{
    using std::runtime_error::runtime_error;
};

}   // namespace fair::mq

// using FairMQSocket [[deprecated("Use fair::mq::Socket")]] = fair::mq::Socket;
// using FairMQSocketPtr [[deprecated("Use fair::mq::SocketPtr")]] = fair::mq::SocketPtr;
using FairMQSocket = fair::mq::Socket;
using FairMQSocketPtr = fair::mq::SocketPtr;

#endif   // FAIR_MQ_SOCKET_H
