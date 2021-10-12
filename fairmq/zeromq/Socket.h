/********************************************************************************
 * Copyright (C) 2014-2021 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_ZMQ_SOCKET_H
#define FAIR_MQ_ZMQ_SOCKET_H

#include <FairMQLogger.h>
#include <FairMQMessage.h>
#include <FairMQSocket.h>
#include <fairmq/tools/Strings.h>
#include <fairmq/zeromq/Common.h>
#include <fairmq/zeromq/Context.h>
#include <fairmq/zeromq/Message.h>

#include <zmq.h>

#include <atomic>
#include <memory> // unique_ptr, make_unique

namespace fair::mq::zmq
{

class Socket final : public fair::mq::Socket
{
  public:
    Socket(Context& ctx, const std::string& type, const std::string& name, const std::string& id, FairMQTransportFactory* factory = nullptr)
        : fair::mq::Socket(factory)
        , fCtx(ctx)
        , fSocket(zmq_socket(fCtx.GetZmqCtx(), getConstant(type)))
        , fId(id + "." + name + "." + type)
        , fBytesTx(0)
        , fBytesRx(0)
        , fMessagesTx(0)
        , fMessagesRx(0)
        , fTimeout(100)
    {
        if (fSocket == nullptr) {
            LOG(error) << "Failed creating socket " << fId << ", reason: " << zmq_strerror(errno);
            throw SocketError(tools::ToString("Unavailable transport requested: ", type));
        }

        if (zmq_setsockopt(fSocket, ZMQ_IDENTITY, fId.c_str(), fId.length()) != 0) {
            LOG(error) << "Failed setting ZMQ_IDENTITY socket option, reason: " << zmq_strerror(errno);
        }

        // Tell socket to try and send/receive outstanding messages for <linger> milliseconds before
        // terminating. Default value for ZeroMQ is -1, which is to wait forever.
        int linger = 1000;
        if (zmq_setsockopt(fSocket, ZMQ_LINGER, &linger, sizeof(linger)) != 0) {
            LOG(error) << "Failed setting ZMQ_LINGER socket option, reason: " << zmq_strerror(errno);
        }

        if (zmq_setsockopt(fSocket, ZMQ_SNDTIMEO, &fTimeout, sizeof(fTimeout)) != 0) {
            LOG(error) << "Failed setting ZMQ_SNDTIMEO socket option, reason: " << zmq_strerror(errno);
        }

        if (zmq_setsockopt(fSocket, ZMQ_RCVTIMEO, &fTimeout, sizeof(fTimeout)) != 0) {
            LOG(error) << "Failed setting ZMQ_RCVTIMEO socket option, reason: " << zmq_strerror(errno);
        }

        if (type == "sub") {
            if (zmq_setsockopt(fSocket, ZMQ_SUBSCRIBE, nullptr, 0) != 0) {
                LOG(error) << "Failed setting ZMQ_SUBSCRIBE socket option, reason: " << zmq_strerror(errno);
            }
        }

        LOG(debug) << "Created socket " << GetId();
    }

    Socket(const Socket&) = delete;
    Socket(Socket&&) = delete;
    Socket& operator=(const Socket&) = delete;
    Socket& operator=(Socket&&) = delete;

    std::string GetId() const override { return fId; }

    bool Bind(const std::string& address) override
    {
        // LOG(debug) << "Binding socket " << fId << " on " << address;

        if (zmq_bind(fSocket, address.c_str()) != 0) {
            if (errno == EADDRINUSE) {
                // do not print error in this case, this is handled by FairMQDevice in case no
                // connection could be established after trying a number of random ports from a range.
                return false;
            }
            LOG(error) << "Failed binding socket " << fId << ", address: " << address << ", reason: " << zmq_strerror(errno);
            return false;
        }

        return true;
    }

    bool Connect(const std::string& address) override
    {
        // LOG(debug) << "Connecting socket " << fId << " on " << address;

        if (zmq_connect(fSocket, address.c_str()) != 0) {
            LOG(error) << "Failed connecting socket " << fId << ", address: " << address << ", reason: " << zmq_strerror(errno);
            return false;
        }

        return true;
    }

    bool ShouldRetry(int flags, int timeout, int& elapsed) const
    {
        if ((flags & ZMQ_DONTWAIT) == 0) {
            if (timeout > 0) {
                elapsed += fTimeout;
                if (elapsed >= timeout) {
                    return false;
                }
            }
            return true;
        } else {
            return false;
        }
    }

    int HandleErrors() const
    {
        if (zmq_errno() == ETERM) {
            LOG(debug) << "Terminating socket " << fId;
            return static_cast<int>(TransferCode::error);
        } else {
            LOG(error) << "Failed transfer on socket " << fId << ", errno: " << errno << ", reason: " << zmq_strerror(errno);
            return static_cast<int>(TransferCode::error);
        }
    }

    int64_t Send(MessagePtr& msg, int timeout = -1) override
    {
        int flags = 0;
        if (timeout == 0) {
            flags = ZMQ_DONTWAIT;
        }
        int elapsed = 0;

        int64_t actualBytes = zmq_msg_size(static_cast<Message*>(msg.get())->GetMessage());

        while (true) {
            int nbytes = zmq_msg_send(static_cast<Message*>(msg.get())->GetMessage(), fSocket, flags);
            if (nbytes >= 0) {
                fBytesTx += actualBytes;
                ++fMessagesTx;
                return actualBytes;
            } else if (zmq_errno() == EAGAIN || zmq_errno() == EINTR) {
                if (fCtx.Interrupted()) {
                    return static_cast<int>(TransferCode::interrupted);
                } else if (ShouldRetry(flags, timeout, elapsed)) {
                    continue;
                } else {
                    return static_cast<int>(TransferCode::timeout);
                }
            } else {
                return HandleErrors();
            }
        }
    }

    int64_t Receive(MessagePtr& msg, int timeout = -1) override
    {
        int flags = 0;
        if (timeout == 0) {
            flags = ZMQ_DONTWAIT;
        }
        int elapsed = 0;

        while (true) {
            int nbytes = zmq_msg_recv(static_cast<Message*>(msg.get())->GetMessage(), fSocket, flags);
            if (nbytes >= 0) {
                static_cast<Message*>(msg.get())->Realign();
                int64_t actualBytes = zmq_msg_size(static_cast<Message*>(msg.get())->GetMessage());
                fBytesRx += actualBytes;
                ++fMessagesRx;
                return actualBytes;
            } else if (zmq_errno() == EAGAIN || zmq_errno() == EINTR) {
                if (fCtx.Interrupted()) {
                    return static_cast<int>(TransferCode::interrupted);
                } else if (ShouldRetry(flags, timeout, elapsed)) {
                    continue;
                } else {
                    return static_cast<int>(TransferCode::timeout);
                }
            } else {
                return HandleErrors();
            }
        }
    }

    int64_t Send(std::vector<std::unique_ptr<fair::mq::Message>>& msgVec, int timeout = -1) override
    {
        int flags = 0;
        if (timeout == 0) {
            flags = ZMQ_DONTWAIT;
        }

        const unsigned int vecSize = msgVec.size();

        // Sending vector typicaly handles more then one part
        if (vecSize > 1) {
            int elapsed = 0;

            while (true) {
                int64_t totalSize = 0;
                bool repeat = false;

                for (unsigned int i = 0; i < vecSize; ++i) {
                    int nbytes = zmq_msg_send(static_cast<Message*>(msgVec[i].get())->GetMessage(), fSocket, (i < vecSize - 1) ? ZMQ_SNDMORE | flags : flags);
                    if (nbytes >= 0) {
                        totalSize += nbytes;
                    } else if (zmq_errno() == EAGAIN || zmq_errno() == EINTR) {
                        if (fCtx.Interrupted()) {
                            return static_cast<int>(TransferCode::interrupted);
                        } else if (ShouldRetry(flags, timeout, elapsed)) {
                            repeat = true;
                            break;
                        } else {
                            return static_cast<int>(TransferCode::timeout);
                        }
                    } else {
                        return HandleErrors();
                    }
                }

                if (repeat) {
                    continue;
                }

                // store statistics on how many messages have been sent (handle all parts as a single message)
                ++fMessagesTx;
                fBytesTx += totalSize;
                return totalSize;
            }
        } else if (vecSize == 1) { // If there's only one part, send it as a regular message
            return Send(msgVec.back(), timeout);
        } else { // if the vector is empty, something might be wrong
            LOG(warn) << "Will not send empty vector";
            return static_cast<int>(TransferCode::error);
        }
    }

    int64_t Receive(std::vector<std::unique_ptr<fair::mq::Message>>& msgVec, int timeout = -1) override
    {
        int flags = 0;
        if (timeout == 0) {
            flags = ZMQ_DONTWAIT;
        }
        int elapsed = 0;

        while (true) {
            int64_t totalSize = 0;
            int more = 0;
            bool repeat = false;

            do {
                FairMQMessagePtr part = std::make_unique<Message>(GetTransport());

                int nbytes = zmq_msg_recv(static_cast<Message*>(part.get())->GetMessage(), fSocket, flags);
                if (nbytes >= 0) {
                    static_cast<Message*>(part.get())->Realign();
                    msgVec.push_back(move(part));
                    totalSize += nbytes;
                } else if (zmq_errno() == EAGAIN || zmq_errno() == EINTR) {
                    if (fCtx.Interrupted()) {
                        return static_cast<int>(TransferCode::interrupted);
                    } else if (ShouldRetry(flags, timeout, elapsed)) {
                        repeat = true;
                        break;
                    } else {
                        return static_cast<int>(TransferCode::timeout);
                    }
                } else {
                    return HandleErrors();
                }

                size_t moreSize = sizeof(more);
                zmq_getsockopt(fSocket, ZMQ_RCVMORE, &more, &moreSize);
            } while (more);

            if (repeat) {
                continue;
            }

            // store statistics on how many messages have been received (handle all parts as a single message)
            ++fMessagesRx;
            fBytesRx += totalSize;
            return totalSize;
        }
    }

    void* GetSocket() const { return fSocket; }

    void Close() override
    {
        // LOG(debug) << "Closing socket " << fId;

        if (fSocket == nullptr) {
            return;
        }

        if (zmq_close(fSocket) != 0) {
            LOG(error) << "Failed closing socket " << fId << ", reason: " << zmq_strerror(errno);
        }

        fSocket = nullptr;
    }

    void SetOption(const std::string& option, const void* value, size_t valueSize) override
    {
        if (zmq_setsockopt(fSocket, getConstant(option), value, valueSize) < 0) {
            LOG(error) << "Failed setting socket option, reason: " << zmq_strerror(errno);
        }
    }

    void GetOption(const std::string& option, void* value, size_t* valueSize) override
    {
        if (zmq_getsockopt(fSocket, getConstant(option), value, valueSize) < 0) {
            LOG(error) << "Failed getting socket option, reason: " << zmq_strerror(errno);
        }
    }

    int Events(uint32_t* events) override
    {
        size_t eventsSize = sizeof(uint32_t);
        return zmq_getsockopt(fSocket, ZMQ_EVENTS, events, &eventsSize);
    }

    void SetLinger(int value) override
    {
        if (zmq_setsockopt(fSocket, ZMQ_LINGER, &value, sizeof(value)) < 0) {
            throw SocketError(tools::ToString("failed setting ZMQ_LINGER, reason: ", zmq_strerror(errno)));
        }
    }

    int GetLinger() const override
    {
        int value = 0;
        size_t valueSize = sizeof(value);
        if (zmq_getsockopt(fSocket, ZMQ_LINGER, &value, &valueSize) < 0) {
            throw SocketError(tools::ToString("failed getting ZMQ_LINGER, reason: ", zmq_strerror(errno)));
        }
        return value;
    }

    void SetSndBufSize(int value) override
    {
        if (zmq_setsockopt(fSocket, ZMQ_SNDHWM, &value, sizeof(value)) < 0) {
            throw SocketError(tools::ToString("failed setting ZMQ_SNDHWM, reason: ", zmq_strerror(errno)));
        }
    }

    int GetSndBufSize() const override
    {
        int value = 0;
        size_t valueSize = sizeof(value);
        if (zmq_getsockopt(fSocket, ZMQ_SNDHWM, &value, &valueSize) < 0) {
            throw SocketError(tools::ToString("failed getting ZMQ_SNDHWM, reason: ", zmq_strerror(errno)));
        }
        return value;
    }

    void SetRcvBufSize(int value) override
    {
        if (zmq_setsockopt(fSocket, ZMQ_RCVHWM, &value, sizeof(value)) < 0) {
            throw SocketError(tools::ToString("failed setting ZMQ_RCVHWM, reason: ", zmq_strerror(errno)));
        }
    }

    int GetRcvBufSize() const override
    {
        int value = 0;
        size_t valueSize = sizeof(value);
        if (zmq_getsockopt(fSocket, ZMQ_RCVHWM, &value, &valueSize) < 0) {
            throw SocketError(tools::ToString("failed getting ZMQ_RCVHWM, reason: ", zmq_strerror(errno)));
        }
        return value;
    }

    void SetSndKernelSize(int value) override
    {
        if (zmq_setsockopt(fSocket, ZMQ_SNDBUF, &value, sizeof(value)) < 0) {
            throw SocketError(tools::ToString("failed getting ZMQ_SNDBUF, reason: ", zmq_strerror(errno)));
        }
    }

    int GetSndKernelSize() const override
    {
        int value = 0;
        size_t valueSize = sizeof(value);
        if (zmq_getsockopt(fSocket, ZMQ_SNDBUF, &value, &valueSize) < 0) {
            throw SocketError(tools::ToString("failed getting ZMQ_SNDBUF, reason: ", zmq_strerror(errno)));
        }
        return value;
    }

    void SetRcvKernelSize(int value) override
    {
        if (zmq_setsockopt(fSocket, ZMQ_RCVBUF, &value, sizeof(value)) < 0) {
            throw SocketError(tools::ToString("failed getting ZMQ_RCVBUF, reason: ", zmq_strerror(errno)));
        }
    }

    int GetRcvKernelSize() const override
    {
        int value = 0;
        size_t valueSize = sizeof(value);
        if (zmq_getsockopt(fSocket, ZMQ_RCVBUF, &value, &valueSize) < 0) {
            throw SocketError(tools::ToString("failed getting ZMQ_RCVBUF, reason: ", zmq_strerror(errno)));
        }
        return value;
    }

    unsigned long GetBytesTx() const override { return fBytesTx; }
    unsigned long GetBytesRx() const override { return fBytesRx; }
    unsigned long GetMessagesTx() const override { return fMessagesTx; }
    unsigned long GetMessagesRx() const override { return fMessagesRx; }

    [[deprecated("Use fair::mq::zmq::getConstant() from <fairmq/zeromq/Common.h> instead.")]]
    static int GetConstant(const std::string& constant) { return getConstant(constant); }

    ~Socket() override { Close(); }

  private:
    Context& fCtx;
    void* fSocket;
    std::string fId;
    std::atomic<unsigned long> fBytesTx;
    std::atomic<unsigned long> fBytesRx;
    std::atomic<unsigned long> fMessagesTx;
    std::atomic<unsigned long> fMessagesRx;

    int fTimeout;
};

} // namespace fair::mq::zmq

#endif /* FAIR_MQ_ZMQ_SOCKET_H */
