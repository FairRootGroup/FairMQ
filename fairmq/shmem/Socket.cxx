/********************************************************************************
 * Copyright (C) 2014-2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "Common.h"
#include "Socket.h"
#include "Message.h"
#include "UnmanagedRegion.h"

#include <FairMQLogger.h>
#include <fairmq/Tools.h>

#include <zmq.h>

#include <stdexcept>

using namespace std;

namespace fair
{
namespace mq
{
namespace shmem
{

atomic<bool> Socket::fInterrupted(false);

struct ZMsg
{
    ZMsg()            { int rc __attribute__((unused)) = zmq_msg_init(&fMsg);            assert(rc == 0); }
    explicit ZMsg(size_t size) { int rc __attribute__((unused)) = zmq_msg_init_size(&fMsg, size); assert(rc == 0); }
    ~ZMsg()           { int rc __attribute__((unused)) = zmq_msg_close(&fMsg);           assert(rc == 0); }

    void* Data()      { return zmq_msg_data(&fMsg); }
    size_t Size()     { return zmq_msg_size(&fMsg); }
    zmq_msg_t* Msg()  { return &fMsg; }

    zmq_msg_t fMsg;
};

Socket::Socket(Manager& manager, const string& type, const string& name, const string& id /*= ""*/, void* context, FairMQTransportFactory* fac /*=nullptr*/)
    : fair::mq::Socket{fac}
    , fSocket(nullptr)
    , fManager(manager)
    , fId(id + "." + name + "." + type)
    , fBytesTx(0)
    , fBytesRx(0)
    , fMessagesTx(0)
    , fMessagesRx(0)
    , fSndTimeout(100)
    , fRcvTimeout(100)
{
    assert(context);
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

    if (zmq_setsockopt(fSocket, ZMQ_SNDTIMEO, &fSndTimeout, sizeof(fSndTimeout)) != 0) {
        LOG(error) << "Failed setting ZMQ_SNDTIMEO socket option, reason: " << zmq_strerror(errno);
    }

    if (zmq_setsockopt(fSocket, ZMQ_RCVTIMEO, &fRcvTimeout, sizeof(fRcvTimeout)) != 0) {
        LOG(error) << "Failed setting ZMQ_RCVTIMEO socket option, reason: " << zmq_strerror(errno);
    }

    // if (type == "sub")
    // {
    //     if (zmq_setsockopt(fSocket, ZMQ_SUBSCRIBE, nullptr, 0) != 0)
    //     {
    //         LOG(error) << "Failed setting ZMQ_SUBSCRIBE socket option, reason: " << zmq_strerror(errno);
    //     }
    // }

    if (type == "sub" || type == "pub") {
        LOG(error) << "PUB/SUB socket type is not supported for shared memory transport";
        throw SocketError("PUB/SUB socket type is not supported for shared memory transport");
    }
    LOG(debug) << "Created socket " << GetId();
}

bool Socket::Bind(const string& address)
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

bool Socket::Connect(const string& address)
{
    // LOG(info) << "connecting socket " << fId << " on " << address;
    if (zmq_connect(fSocket, address.c_str()) != 0) {
        LOG(error) << "Failed connecting socket " << fId << ", reason: " << zmq_strerror(errno);
        return false;
    }
    return true;
}

int Socket::Send(MessagePtr& msg, const int timeout)
{
    int flags = 0;
    if (timeout == 0) {
        flags = ZMQ_DONTWAIT;
    }
    int elapsed = 0;

    Message* shmMsg = static_cast<Message*>(msg.get());
    ZMsg zmqMsg(sizeof(MetaHeader));
    std::memcpy(zmqMsg.Data(), &(shmMsg->fMeta), sizeof(MetaHeader));

    while (true && !fInterrupted) {
        int nbytes = zmq_msg_send(zmqMsg.Msg(), fSocket, flags);
        if (nbytes > 0) {
            shmMsg->fQueued = true;
            ++fMessagesTx;
            size_t size = msg->GetSize();
            fBytesTx += size;
            return size;
        } else if (zmq_errno() == EAGAIN) {
            if (!fInterrupted && ((flags & ZMQ_DONTWAIT) == 0)) {
                if (timeout > 0) {
                    elapsed += fSndTimeout;
                    if (elapsed >= timeout) {
                        return -2;
                    }
                }
                continue;
            } else {
                return -2;
            }
        } else if (zmq_errno() == ETERM) {
            LOG(info) << "terminating socket " << fId;
            return -1;
        } else {
            LOG(error) << "Failed sending on socket " << fId << ", reason: " << zmq_strerror(errno) << ", nbytes = " << nbytes;
            return nbytes;
        }
    }

    return -1;
}

int Socket::Receive(MessagePtr& msg, const int timeout)
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
            assert((nbytes / sizeof(MetaHeader)) == 1);

            MetaHeader* hdr = static_cast<MetaHeader*>(zmqMsg.Data());
            size_t size = hdr->fSize;
            shmMsg->fMeta = *hdr;

            fBytesRx += size;
            ++fMessagesRx;
            return size;
        } else if (zmq_errno() == EAGAIN) {
            if (!fInterrupted && ((flags & ZMQ_DONTWAIT) == 0)) {
                if (timeout > 0) {
                    elapsed += fRcvTimeout;
                    if (elapsed >= timeout) {
                        return -2;
                    }
                }
                continue;
            } else {
                return -2;
            }
        } else if (zmq_errno() == ETERM) {
            LOG(info) << "terminating socket " << fId;
            return -1;
        } else {
            LOG(error) << "Failed receiving on socket " << fId << ", reason: " << zmq_strerror(errno) << ", nbytes = " << nbytes;
            return nbytes;
        }
    }
}

int64_t Socket::Send(vector<MessagePtr>& msgVec, const int timeout)
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

    while (!fInterrupted) {
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
        } else if (zmq_errno() == EAGAIN) {
            if (!fInterrupted && ((flags & ZMQ_DONTWAIT) == 0)) {
                if (timeout > 0) {
                    elapsed += fSndTimeout;
                    if (elapsed >= timeout) {
                        return -2;
                    }
                }
                continue;
            } else {
                return -2;
            }
        } else if (zmq_errno() == ETERM) {
            LOG(info) << "terminating socket " << fId;
            return -1;
        } else {
            LOG(error) << "Failed sending on socket " << fId << ", reason: " << zmq_strerror(errno) << ", nbytes = " << nbytes;
            return nbytes;
        }
    }

    return -1;
}

int64_t Socket::Receive(vector<MessagePtr>& msgVec, const int timeout)
{
    int flags = 0;
    if (timeout == 0) {
        flags = ZMQ_DONTWAIT;
    }
    int elapsed = 0;

    ZMsg zmqMsg;

    while (!fInterrupted) {
        int64_t totalSize = 0;
        int nbytes = zmq_msg_recv(zmqMsg.Msg(), fSocket, flags);
        if (nbytes > 0) {
            MetaHeader* hdrVec = static_cast<MetaHeader*>(zmqMsg.Data());
            const auto hdrVecSize = zmqMsg.Size();
            assert(hdrVecSize > 0);
            assert(hdrVecSize % sizeof(MetaHeader) == 0);

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
        } else if (zmq_errno() == EAGAIN) {
            if (!fInterrupted && ((flags & ZMQ_DONTWAIT) == 0)) {
                if (timeout > 0) {
                    elapsed += fRcvTimeout;
                    if (elapsed >= timeout) {
                        return -2;
                    }
                }
                continue;
            } else {
                return -2;
            }
        } else {
            LOG(error) << "Failed receiving on socket " << fId << ", reason: " << zmq_strerror(errno) << ", nbytes = " << nbytes;
            return nbytes;
        }
    }

    return -1;
}

void Socket::Close()
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

void Socket::Interrupt()
{
    Manager::Interrupt();
    Message::fInterrupted = true;
    fInterrupted = true;
}

void Socket::Resume()
{
    Manager::Resume();
    Message::fInterrupted = false;
    fInterrupted = false;
}

void Socket::SetOption(const string& option, const void* value, size_t valueSize)
{
    if (zmq_setsockopt(fSocket, GetConstant(option), value, valueSize) < 0) {
        LOG(error) << "Failed setting socket option, reason: " << zmq_strerror(errno);
    }
}

void Socket::GetOption(const string& option, void* value, size_t* valueSize)
{
    if (zmq_getsockopt(fSocket, GetConstant(option), value, valueSize) < 0) {
        LOG(error) << "Failed getting socket option, reason: " << zmq_strerror(errno);
    }
}

void Socket::SetLinger(const int value)
{
    if (zmq_setsockopt(fSocket, ZMQ_LINGER, &value, sizeof(value)) < 0) {
        throw SocketError(tools::ToString("failed setting ZMQ_LINGER, reason: ", zmq_strerror(errno)));
    }
}

int Socket::GetLinger() const
{
    int value = 0;
    size_t valueSize = sizeof(value);
    if (zmq_getsockopt(fSocket, ZMQ_LINGER, &value, &valueSize) < 0) {
        throw SocketError(tools::ToString("failed getting ZMQ_LINGER, reason: ", zmq_strerror(errno)));
    }
    return value;
}

void Socket::SetSndBufSize(const int value)
{
    if (zmq_setsockopt(fSocket, ZMQ_SNDHWM, &value, sizeof(value)) < 0) {
        throw SocketError(tools::ToString("failed setting ZMQ_SNDHWM, reason: ", zmq_strerror(errno)));
    }
}

int Socket::GetSndBufSize() const
{
    int value = 0;
    size_t valueSize = sizeof(value);
    if (zmq_getsockopt(fSocket, ZMQ_SNDHWM, &value, &valueSize) < 0) {
        throw SocketError(tools::ToString("failed getting ZMQ_SNDHWM, reason: ", zmq_strerror(errno)));
    }
    return value;
}

void Socket::SetRcvBufSize(const int value)
{
    if (zmq_setsockopt(fSocket, ZMQ_RCVHWM, &value, sizeof(value)) < 0) {
        throw SocketError(tools::ToString("failed setting ZMQ_RCVHWM, reason: ", zmq_strerror(errno)));
    }
}

int Socket::GetRcvBufSize() const
{
    int value = 0;
    size_t valueSize = sizeof(value);
    if (zmq_getsockopt(fSocket, ZMQ_RCVHWM, &value, &valueSize) < 0) {
        throw SocketError(tools::ToString("failed getting ZMQ_RCVHWM, reason: ", zmq_strerror(errno)));
    }
    return value;
}

void Socket::SetSndKernelSize(const int value)
{
    if (zmq_setsockopt(fSocket, ZMQ_SNDBUF, &value, sizeof(value)) < 0) {
        throw SocketError(tools::ToString("failed getting ZMQ_SNDBUF, reason: ", zmq_strerror(errno)));
    }
}

int Socket::GetSndKernelSize() const
{
    int value = 0;
    size_t valueSize = sizeof(value);
    if (zmq_getsockopt(fSocket, ZMQ_SNDBUF, &value, &valueSize) < 0) {
        throw SocketError(tools::ToString("failed getting ZMQ_SNDBUF, reason: ", zmq_strerror(errno)));
    }
    return value;
}

void Socket::SetRcvKernelSize(const int value)
{
    if (zmq_setsockopt(fSocket, ZMQ_RCVBUF, &value, sizeof(value)) < 0) {
        throw SocketError(tools::ToString("failed getting ZMQ_RCVBUF, reason: ", zmq_strerror(errno)));
    }
}

int Socket::GetRcvKernelSize() const
{
    int value = 0;
    size_t valueSize = sizeof(value);
    if (zmq_getsockopt(fSocket, ZMQ_RCVBUF, &value, &valueSize) < 0) {
        throw SocketError(tools::ToString("failed getting ZMQ_RCVBUF, reason: ", zmq_strerror(errno)));
    }
    return value;
}

int Socket::GetConstant(const string& constant)
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

    return -1;
}

}
}
}
