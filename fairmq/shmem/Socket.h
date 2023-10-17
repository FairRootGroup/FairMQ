/********************************************************************************
 * Copyright (C) 2014-2023 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
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
#include <fairmq/Error.h>              // for assertm
#include <fairmq/Message.h>
#include <fairmq/Socket.h>
#include <fairmq/tools/Strings.h>
#include <fairmq/zeromq/Common.h>      // for zmq::HandleErrors, zmq::ShouldRetry
#include <fairmq/zeromq/ZMsg.h>        // for zmq::ZMsg

#include <fairlogger/Logger.h>

#include <zmq.h>

#include <algorithm>         // for std::max
#include <atomic>
#include <cstddef>           // for std::size_t
#include <cstring>           // for std::memcpy
#include <exception>         // for std::terminate
#include <memory>            // for std::make_unique

namespace fair::mq {
    class TransportFactory;
}

namespace fair::mq::shmem
{

class Socket final : public fair::mq::Socket
{
  public:
    Socket(Manager& manager, const std::string& type, const std::string& name, const std::string& id, void* context, fair::mq::TransportFactory* fac = nullptr)
        : fair::mq::Socket(fac)
        , fManager(manager)
        , fId(id + "." + name + "." + type)
        , fSocket(nullptr)
        , fMonitorSocket(nullptr)
        , fBytesTx(0)
        , fBytesRx(0)
        , fMessagesTx(0)
        , fMessagesRx(0)
        , fTimeout(100)
        , fConnectedPeersCount(0)
        , fMetadataMsgSize(manager.GetMetadataMsgSize())
    {
        assert(context);

        if (type == "sub" || type == "pub") {
            LOG(error) << "PUB/SUB socket type is not supported for shared memory transport";
            throw SocketError("PUB/SUB socket type is not supported for shared memory transport");
        }

        fSocket = zmq_socket(context, zmq::getConstant(type));
        fMonitorSocket = zmq::makeMonitorSocket(context, fSocket, fId);

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
    Socket(Socket&&) = delete;
    Socket& operator=(const Socket&) = delete;
    Socket& operator=(Socket&&) = delete;

    std::string GetId() const override { return fId; }

    bool Bind(const std::string& address) override
    {
        return zmq::Bind(fSocket, address, fId);
    }

    bool Connect(const std::string& address) override
    {
        return zmq::Connect(fSocket, address, fId);
    }

    int64_t Send(mq::MessagePtr& msg, int timeout = -1) override
    {
        auto msgPtr = msg.get();
        if (!msgPtr) {
            return static_cast<int>(TransferCode::error);
        }
        assertm(dynamic_cast<shmem::Message*>(msgPtr), "given mq::Message is a shmem::Message");   // NOLINT
        auto shmMsg = static_cast<shmem::Message*>(msgPtr);   // NOLINT(cppcoreguidelines-pro-type-static-cast-downcast)

        int flags = 0;
        if (timeout == 0) {
            flags = ZMQ_DONTWAIT;
        }
        int elapsed = 0;

        MetaHeader meta{ shmMsg->fSize, shmMsg->fHint, shmMsg->fHandle, shmMsg->fShared, shmMsg->fRegionId, shmMsg->fSegmentId, shmMsg->fManaged };

        // meta msg format: | MetaHeader | padded to fMetadataMsgSize |
        zmq::ZMsg zmqMsg(std::max(fMetadataMsgSize, sizeof(MetaHeader)));
        std::memcpy(zmqMsg.Data(), &meta, sizeof(MetaHeader));

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
                } else if (zmq::ShouldRetry(flags, fTimeout, timeout, elapsed)) {
                    continue;
                } else {
                    return static_cast<int>(TransferCode::timeout);
                }
            } else {
                return zmq::HandleErrors(fId);
            }
        }

        return static_cast<int>(TransferCode::error);
    }

    int64_t Receive(MessagePtr& msg, int timeout = -1) override
    {
        int flags = 0;
        if (timeout == 0) {
            flags = ZMQ_DONTWAIT;
        }
        int elapsed = 0;

        while (true) {
            Message* shmMsg = static_cast<Message*>(msg.get());
            MetaHeader meta;
            int nbytes = zmq_recv(fSocket, &meta, sizeof(MetaHeader), flags);
            if (nbytes > 0) {
                // check for number of received messages. must be 1
                if (static_cast<std::size_t>(nbytes) < sizeof(MetaHeader)) {
                    throw SocketError(
                        tools::ToString("Received message is not a valid FairMQ shared memory message. ",
                            "Possibly due to a misconfigured transport on the sender side. ",
                            "Expected minimum size of ", sizeof(MetaHeader), " bytes, received ", nbytes));
                }

                shmMsg->SetMeta(meta);

                size_t size = shmMsg->GetSize();
                fBytesRx += size;
                ++fMessagesRx;
                return size;
            } else if (zmq_errno() == EAGAIN || zmq_errno() == EINTR) {
                if (fManager.Interrupted()) {
                    return static_cast<int>(TransferCode::interrupted);
                } else if (zmq::ShouldRetry(flags, fTimeout, timeout, elapsed)) {
                    continue;
                } else {
                    return static_cast<int>(TransferCode::timeout);
                }
            } else {
                return zmq::HandleErrors(fId);
            }
        }
    }

    int64_t Send(std::vector<MessagePtr>& msgVec, int timeout = -1) override
    {
        int flags = 0;
        if (timeout == 0) {
            flags = ZMQ_DONTWAIT;
        }
        int elapsed = 0;

        // meta msg format: | n | MetaHeader 1 | ... | MetaHeader n | padded to fMetadataMsgSize |
        auto const n = msgVec.size();
        zmq::ZMsg zmqMsg(std::max(fMetadataMsgSize, sizeof(std::size_t) + n * sizeof(MetaHeader)));

        auto meta_n = static_cast<std::size_t*>(zmqMsg.Data());
        *meta_n = n;
        ++meta_n;
        auto metas = static_cast<MetaHeader*>(static_cast<void*>(meta_n));
        for (auto& msg : msgVec) {
            auto msgPtr = msg.get();
            if (!msgPtr) {
                return static_cast<int>(TransferCode::error);
            }
            assertm(dynamic_cast<shmem::Message*>(msgPtr), "given mq::Message is a shmem::Message");   // NOLINT
            auto shmMsg = static_cast<shmem::Message*>(msgPtr);   // NOLINT(cppcoreguidelines-pro-type-static-cast-downcast)
            MetaHeader meta{ shmMsg->fSize, shmMsg->fHint, shmMsg->fHandle, shmMsg->fShared, shmMsg->fRegionId, shmMsg->fSegmentId, shmMsg->fManaged };
            std::memcpy(metas++, &meta, sizeof(MetaHeader));
        }

        while (true) {
            int64_t totalSize = 0;
            int nbytes = zmq_msg_send(zmqMsg.Msg(), fSocket, flags);
            if (nbytes > 0) {
                assert(static_cast<unsigned int>(nbytes) >= sizeof(std::size_t) + (n * sizeof(MetaHeader)));

                for (auto& msg : msgVec) {
                    Message* shmMsg = static_cast<Message*>(msg.get());
                    shmMsg->fQueued = true;
                    totalSize += shmMsg->fSize;
                }

                // store statistics on how many messages have been sent
                fMessagesTx++;
                fBytesTx += totalSize;

                return totalSize;
            } else if (zmq_errno() == EAGAIN || zmq_errno() == EINTR) {
                if (fManager.Interrupted()) {
                    return static_cast<int>(TransferCode::interrupted);
                } else if (zmq::ShouldRetry(flags, fTimeout, timeout, elapsed)) {
                    continue;
                } else {
                    return static_cast<int>(TransferCode::timeout);
                }
            } else {
                return zmq::HandleErrors(fId);
            }
        }

        return static_cast<int>(TransferCode::error);
    }

    int64_t Receive(std::vector<MessagePtr>& msgVec, int timeout = -1) override
    {
        int flags = 0;
        if (timeout == 0) {
            flags = ZMQ_DONTWAIT;
        }
        int elapsed = 0;

        zmq::ZMsg zmqMsg;

        while (true) {
            std::size_t totalSize = 0;
            int nbytes = zmq_msg_recv(zmqMsg.Msg(), fSocket, flags);
            if (nbytes > 0) {
                [[maybe_unused]] auto const size = zmqMsg.Size();
                assert(size > sizeof(std::size_t));
                auto meta_n = static_cast<std::size_t*>(zmqMsg.Data());
                auto const n = *meta_n;
                assert(size >= sizeof(std::size_t) + n * sizeof(MetaHeader));
                ++meta_n;
                auto metas = static_cast<MetaHeader*>(static_cast<void*>(meta_n));
                msgVec.reserve(msgVec.size() + n);
                auto const transport = GetTransport();

                for (std::size_t i = 0; i < n; ++i) {
                    msgVec.push_back(std::make_unique<Message>(fManager, *metas, transport));
                    ++metas;
                    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
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
                } else if (zmq::ShouldRetry(flags, fTimeout, timeout, elapsed)) {
                    continue;
                } else {
                    return static_cast<int>(TransferCode::timeout);
                }
            } else {
                return zmq::HandleErrors(fId);
            }
        }

        return static_cast<int>(TransferCode::error);
    }

    void* GetSocket() const { return fSocket; }

    void Close() override
    {
        // LOG(debug) << "Closing socket " << fId;

        if (fSocket && zmq_close(fSocket) != 0) {
            LOG(error) << "Failed closing data socket " << fId
                       << ", reason: " << zmq_strerror(errno);
        }
        fSocket = nullptr;

        if (fMonitorSocket && zmq_close(fMonitorSocket) != 0) {
            LOG(error) << "Failed closing monitor socket " << fId
                       << ", reason: " << zmq_strerror(errno);
        }
        fMonitorSocket = nullptr;
    }

    void SetOption(const std::string& option, const void* value, size_t valueSize) override
    {
        if (zmq_setsockopt(fSocket, zmq::getConstant(option), value, valueSize) < 0) {
            LOG(error) << "Failed setting socket option, reason: " << zmq_strerror(errno);
        }
    }

    void GetOption(const std::string& option, void* value, size_t* valueSize) override
    {
        if (zmq_getsockopt(fSocket, zmq::getConstant(option), value, valueSize) < 0) {
            LOG(error) << "Failed getting socket option, reason: " << zmq_strerror(errno);
        }
    }

    void SetLinger(int value) override
    {
        if (zmq_setsockopt(fSocket, ZMQ_LINGER, &value, sizeof(value)) < 0) {
            throw SocketError(tools::ToString("failed setting ZMQ_LINGER, reason: ", zmq_strerror(errno)));
        }
    }

    int Events(uint32_t* events) override
    {
        size_t eventsSize = sizeof(uint32_t);
        return zmq_getsockopt(fSocket, ZMQ_EVENTS, events, &eventsSize);
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

    unsigned long GetNumberOfConnectedPeers() const override
    {
        fConnectedPeersCount = zmq::updateNumberOfConnectedPeers(fConnectedPeersCount, fMonitorSocket);
        return fConnectedPeersCount;
    }

    unsigned long GetBytesTx() const override { return fBytesTx; }
    unsigned long GetBytesRx() const override { return fBytesRx; }
    unsigned long GetMessagesTx() const override { return fMessagesTx; }
    unsigned long GetMessagesRx() const override { return fMessagesRx; }

    [[deprecated("Use fair::mq::zmq::getConstant() from <fairmq/zeromq/Common.h> instead.")]]
    static int GetConstant(const std::string& constant) { return zmq::getConstant(constant); }

    ~Socket() override { Close(); }

  private:
    Manager& fManager;
    std::string fId;
    void* fSocket;
    void* fMonitorSocket;
    std::atomic<unsigned long> fBytesTx;
    std::atomic<unsigned long> fBytesRx;
    std::atomic<unsigned long> fMessagesTx;
    std::atomic<unsigned long> fMessagesRx;

    int fTimeout;
    mutable unsigned long fConnectedPeersCount;
    std::size_t fMetadataMsgSize;
};

} // namespace fair::mq::shmem

#endif /* FAIR_MQ_SHMEM_SOCKET_H_ */
