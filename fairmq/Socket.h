/********************************************************************************
 * Copyright (C) 2021-2022 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_SOCKET_H
#define FAIR_MQ_SOCKET_H

#include <fairmq/Message.h>
#include <fairmq/Parts.h>

#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
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

template <typename T>
struct is_transferrable : std::disjunction<std::is_same<T, MessagePtr>,
                                           std::is_same<T, std::vector<MessagePtr>>,
                                           std::is_same<T, fair::mq::Parts>>
{};

struct Socket
{
    Socket() = default;
    Socket(TransportFactory* fac) : fTransport(fac) {}
    Socket(const Socket&) = delete;
    Socket(Socket&&) = delete;
    Socket& operator=(const Socket&) = delete;
    Socket& operator=(Socket&&) = delete;

    virtual std::string GetId() const = 0;

    virtual bool Bind(const std::string& address) = 0;
    virtual bool Connect(const std::string& address) = 0;

    virtual int64_t Send(MessagePtr& msg, int timeout = -1) = 0;
    virtual int64_t Receive(MessagePtr& msg, int timeout = -1) = 0;
    virtual int64_t Send(Parts::container& msgVec, int timeout = -1) = 0;
    virtual int64_t Receive(Parts::container & msgVec, int timeout = -1) = 0;
    virtual int64_t Send(Parts& parts, int timeout = -1) { return Send(parts.fParts, timeout); }
    virtual int64_t Receive(Parts& parts, int timeout = -1) { return Receive(parts.fParts, timeout); }

    [[deprecated("Use Socket::~Socket() instead.")]]
    virtual void Close() = 0;

    virtual void SetOption(const std::string& option, const void* value, size_t valueSize) = 0;
    virtual void GetOption(const std::string& option, void* value, size_t* valueSize) = 0;

    /// If the backend supports it, fills the unsigned integer @a events with the ZMQ_EVENTS value
    /// DISCLAIMER: this API is experimental and unsupported and might be dropped / refactored in
    /// the future.
    virtual int Events(uint32_t* events) = 0;
    virtual void SetLinger(int value) = 0;
    virtual int GetLinger() const = 0;
    virtual void SetSndBufSize(int value) = 0;
    virtual int GetSndBufSize() const = 0;
    virtual void SetRcvBufSize(int value) = 0;
    virtual int GetRcvBufSize() const = 0;
    virtual void SetSndKernelSize(int value) = 0;
    virtual int GetSndKernelSize() const = 0;
    virtual void SetRcvKernelSize(int value) = 0;
    virtual int GetRcvKernelSize() const = 0;

    virtual unsigned long GetBytesTx() const = 0;
    virtual unsigned long GetBytesRx() const = 0;
    virtual unsigned long GetMessagesTx() const = 0;
    virtual unsigned long GetMessagesRx() const = 0;

    virtual unsigned long GetNumberOfConnectedPeers() const = 0;

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

using FairMQSocket [[deprecated("Use fair::mq::Socket")]] = fair::mq::Socket;
using FairMQSocketPtr [[deprecated("Use fair::mq::SocketPtr")]] = fair::mq::SocketPtr;

#endif   // FAIR_MQ_SOCKET_H
