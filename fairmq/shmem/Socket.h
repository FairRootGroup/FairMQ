/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
#ifndef FAIR_MQ_SHMEM_SOCKET_H_
#define FAIR_MQ_SHMEM_SOCKET_H_

#include "Common.h"
#include "Manager.h"
#include "Message.h"

#include <FairMQSocket.h>
#include <FairMQMessage.h>
#include <FairMQLogger.h>
#include <fairmq/Tools.h>

#include <zmq.h>

#include <atomic>

class FairMQTransportFactory;

namespace fair
{
namespace mq
{
namespace shmem
{

struct ZMsg
{
    ZMsg() { int rc __attribute__((unused)) = zmq_msg_init(&fMsg); assert(rc == 0); }
    explicit ZMsg(size_t size) { int rc __attribute__((unused)) = zmq_msg_init_size(&fMsg, size); assert(rc == 0); }
    ~ZMsg() { int rc __attribute__((unused)) = zmq_msg_close(&fMsg); assert(rc == 0); }

    void* Data() { return zmq_msg_data(&fMsg); }
    size_t Size() { return zmq_msg_size(&fMsg); }
    zmq_msg_t* Msg() { return &fMsg; }

    zmq_msg_t fMsg;
};

class Socket final : public fair::mq::Socket
{
  public:
    Socket(Manager& manager, const std::string& type, const std::string& name, const std::string& id, void* context, FairMQTransportFactory* fac = nullptr)
        : fair::mq::Socket(fac)
        , fSocket(nullptr)
        , fManager(manager)
        , fId(id + "." + name + "." + type)
        , fBytesTx(0)
        , fBytesRx(0)
        , fMessagesTx(0)
        , fMessagesRx(0)
        , fTimeout(100)
    {
        assert(context);

        if (type == "sub" || type == "pub") {
            LOG(error) << "PUB/SUB socket type is not supported for shared memory transport";
            throw SocketError("PUB/SUB socket type is not supported for shared memory transport");
        }

        fSocket = zmq_socket(context, GetConstant(type));

        if (fSocket == nullptr) {
            LOG(error) << "Failed creating socket " << fId << ", reason: " << zmq_strerror(errno);
            throw SocketError(tools::ToString("Failed creating socket ", fId, ", reason: ", zmq_strerror(errno)));
        }

        if (zmq_setsockopt(fSocket, ZMQ_IDENTITY, fId.c_str(), fId.length()) != 0) {
            LOG(error) << "Failed setting ZMQ_IDENTITY socket option, reason: " << zmq_strerror(errno);
        }

        // Tell socket to try and send/receive outstanding messages for <linger> milliseconds before terminating.
        // Default value for ZeroMQ is -1, which is to wait forever.
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

        // if (type == "sub")
        // {
        //     if (zmq_setsockopt(fSocket, ZMQ_SUBSCRIBE, nullptr, 0) != 0)
        //     {
        //         LOG(error) << "Failed setting ZMQ_SUBSCRIBE socket option, reason: " << zmq_strerror(errno);
        //     }
        // }
        LOG(debug) << "Created socket " << GetId();
    }

    Socket(const Socket&) = delete;
    Socket operator=(const Socket&) = delete;

    std::string GetId() const override { return fId; }

    bool Bind(const std::string& address) override
    {
        // LOG(info) << "binding socket " << fId << " on " << address;
        if (zmq_bind(fSocket, address.c_str()) != 0) {
            if (errno == EADDRINUSE) {
                // do not print error in this case, this is handled by FairMQDevice in case no connection could be established after trying a number of random ports from a range.
                return false;
            }
            LOG(error) << "Failed binding socket " << fId << ", reason: " << zmq_strerror(errno);
            return false;
        }
        return true;
    }

    bool Connect(const std::string& address) override
    {
        // LOG(info) << "connecting socket " << fId << " on " << address;
        if (zmq_connect(fSocket, address.c_str()) != 0) {
            LOG(error) << "Failed connecting socket " << fId << ", reason: " << zmq_strerror(errno);
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
            LOG(error) << "Failed transfer on socket " << fId << ", reason: " << zmq_strerror(errno);
            return static_cast<int>(TransferCode::error);
        }
    }

    int64_t Send(MessagePtr& msg, const int timeout = -1) override
    {
        int flags = 0;
        if (timeout == 0) {
            flags = ZMQ_DONTWAIT;
        }
        int elapsed = 0;

        Message* shmMsg = static_cast<Message*>(msg.get());
        ZMsg zmqMsg(sizeof(MetaHeader));
        std::memcpy(zmqMsg.Data(), &(shmMsg->fMeta), sizeof(MetaHeader));

        while (true) {
            int nbytes = zmq_msg_send(zmqMsg.Msg(), fSocket, flags);
            if (nbytes > 0) {
                shmMsg->fQueued = true;
                ++fMessagesTx;
                size_t size = msg->GetSize();
                fBytesTx += size;
                return size;
            } else if (zmq_errno() == EAGAIN || zmq_errno() == EINTR) {
                if (fManager.Interrupted()) {
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

        return static_cast<int>(TransferCode::error);
    }

    int64_t Receive(MessagePtr& msg, const int timeout = -1) override
    {
        int flags = 0;
        if (timeout == 0) {
            flags = ZMQ_DONTWAIT;
        }
        int elapsed = 0;

        ZMsg zmqMsg;

        while (true) {
            Message* shmMsg = static_cast<Message*>(msg.get());
            int nbytes = zmq_msg_recv(zmqMsg.Msg(), fSocket, flags);
            if (nbytes > 0) {
                // check for number of received messages. must be 1
                if (nbytes != sizeof(MetaHeader)) {
                    throw SocketError(
                        tools::ToString("Received message is not a valid FairMQ shared memory message. ",
                            "Possibly due to a misconfigured transport on the sender side. ",
                            "Expected size of ", sizeof(MetaHeader), " bytes, received ", nbytes));
                }

                MetaHeader* hdr = static_cast<MetaHeader*>(zmqMsg.Data());
                size_t size = hdr->fSize;
                shmMsg->fMeta = *hdr;

                fBytesRx += size;
                ++fMessagesRx;
                return size;
            } else if (zmq_errno() == EAGAIN || zmq_errno() == EINTR) {
                if (fManager.Interrupted()) {
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

    int64_t Send(std::vector<MessagePtr>& msgVec, const int timeout = -1) override
    {
        int flags = 0;
        if (timeout == 0) {
            flags = ZMQ_DONTWAIT;
        }
        int elapsed = 0;

        // put it into zmq message
        const unsigned int vecSize = msgVec.size();
        ZMsg zmqMsg(vecSize * sizeof(MetaHeader));

        // prepare the message with shm metas
        MetaHeader* metas = static_cast<MetaHeader*>(zmqMsg.Data());

        for (auto& msg : msgVec) {
            Message* shmMsg = static_cast<Message*>(msg.get());
            std::memcpy(metas++, &(shmMsg->fMeta), sizeof(MetaHeader));
        }

        while (true) {
            int64_t totalSize = 0;
            int nbytes = zmq_msg_send(zmqMsg.Msg(), fSocket, flags);
            if (nbytes > 0) {
                assert(static_cast<unsigned int>(nbytes) == (vecSize * sizeof(MetaHeader))); // all or nothing

                for (auto& msg : msgVec) {
                    Message* shmMsg = static_cast<Message*>(msg.get());
                    shmMsg->fQueued = true;
                    totalSize += shmMsg->fMeta.fSize;
                }

                // store statistics on how many messages have been sent
                fMessagesTx++;
                fBytesTx += totalSize;

                return totalSize;
            } else if (zmq_errno() == EAGAIN || zmq_errno() == EINTR) {
                if (fManager.Interrupted()) {
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

        return static_cast<int>(TransferCode::error);
    }

    int64_t Receive(std::vector<MessagePtr>& msgVec, const int timeout = -1) override
    {
        int flags = 0;
        if (timeout == 0) {
            flags = ZMQ_DONTWAIT;
        }
        int elapsed = 0;

        ZMsg zmqMsg;

        while (true) {
            int64_t totalSize = 0;
            int nbytes = zmq_msg_recv(zmqMsg.Msg(), fSocket, flags);
            if (nbytes > 0) {
                MetaHeader* hdrVec = static_cast<MetaHeader*>(zmqMsg.Data());
                const auto hdrVecSize = zmqMsg.Size();

                assert(hdrVecSize > 0);
                if (hdrVecSize % sizeof(MetaHeader) != 0) {
                    throw SocketError(
                        tools::ToString("Received message is not a valid FairMQ shared memory message. ",
                            "Possibly due to a misconfigured transport on the sender side. ",
                            "Expected size of ", sizeof(MetaHeader), " bytes, received ", nbytes));
                }

                const auto numMessages = hdrVecSize / sizeof(MetaHeader);
                msgVec.reserve(numMessages);

                for (size_t m = 0; m < numMessages; m++) {
                    // create new message (part)
                    msgVec.emplace_back(tools::make_unique<Message>(fManager, hdrVec[m], GetTransport()));
                    Message* shmMsg = static_cast<Message*>(msgVec.back().get());
                    totalSize += shmMsg->GetSize();
                }

                // store statistics on how many messages have been received (handle all parts as a single message)
                fMessagesRx++;
                fBytesRx += totalSize;

                return totalSize;
            } else if (zmq_errno() == EAGAIN || zmq_errno() == EINTR) {
                if (fManager.Interrupted()) {
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

        return static_cast<int>(TransferCode::error);
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
        if (zmq_setsockopt(fSocket, GetConstant(option), value, valueSize) < 0) {
            LOG(error) << "Failed setting socket option, reason: " << zmq_strerror(errno);
        }
    }

    void GetOption(const std::string& option, void* value, size_t* valueSize) override
    {
        if (zmq_getsockopt(fSocket, GetConstant(option), value, valueSize) < 0) {
            LOG(error) << "Failed getting socket option, reason: " << zmq_strerror(errno);
        }
    }

    void SetLinger(const int value) override
    {
        if (zmq_setsockopt(fSocket, ZMQ_LINGER, &value, sizeof(value)) < 0) {
            throw SocketError(tools::ToString("failed setting ZMQ_LINGER, reason: ", zmq_strerror(errno)));
        }
    }

    void Events(uint32_t* events) override
    {
        size_t eventsSize = sizeof(uint32_t);
        if (zmq_getsockopt(fSocket, ZMQ_EVENTS, events, &eventsSize) < 0) {
            throw SocketError(
                tools::ToString("failed setting ZMQ_EVENTS, reason: ", zmq_strerror(errno)));
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

    void SetSndBufSize(const int value) override
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

    void SetRcvBufSize(const int value) override
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

    void SetSndKernelSize(const int value) override
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

    void SetRcvKernelSize(const int value) override
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

    static int GetConstant(const std::string& constant)
    {
        if (constant == "") return 0;
        if (constant == "sub") return ZMQ_SUB;
        if (constant == "pub") return ZMQ_PUB;
        if (constant == "xsub") return ZMQ_XSUB;
        if (constant == "xpub") return ZMQ_XPUB;
        if (constant == "push") return ZMQ_PUSH;
        if (constant == "pull") return ZMQ_PULL;
        if (constant == "req") return ZMQ_REQ;
        if (constant == "rep") return ZMQ_REP;
        if (constant == "dealer") return ZMQ_DEALER;
        if (constant == "router") return ZMQ_ROUTER;
        if (constant == "pair") return ZMQ_PAIR;

        if (constant == "snd-hwm") return ZMQ_SNDHWM;
        if (constant == "rcv-hwm") return ZMQ_RCVHWM;
        if (constant == "snd-size") return ZMQ_SNDBUF;
        if (constant == "rcv-size") return ZMQ_RCVBUF;
        if (constant == "snd-more") return ZMQ_SNDMORE;
        if (constant == "rcv-more") return ZMQ_RCVMORE;

        if (constant == "linger") return ZMQ_LINGER;
        if (constant == "no-block") return ZMQ_DONTWAIT;
        if (constant == "snd-more no-block") return ZMQ_DONTWAIT|ZMQ_SNDMORE;

        if (constant == "fd") return ZMQ_FD;
        if (constant == "events")
            return ZMQ_EVENTS;
        if (constant == "pollin")
            return ZMQ_POLLIN;
        if (constant == "pollout")
            return ZMQ_POLLOUT;

        throw SocketError(tools::ToString("GetConstant called with an invalid argument: ", constant));
    }

    ~Socket() override { Close(); }

  private:
    void* fSocket;
    Manager& fManager;
    std::string fId;
    std::atomic<unsigned long> fBytesTx;
    std::atomic<unsigned long> fBytesRx;
    std::atomic<unsigned long> fMessagesTx;
    std::atomic<unsigned long> fMessagesRx;

    int fTimeout;
};

}
}
}

#endif /* FAIR_MQ_SHMEM_SOCKET_H_ */
