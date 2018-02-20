/********************************************************************************
 *    Copyright (C) 2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <fairmq/ofi/Socket.h>
#include <fairmq/ofi/TransportFactory.h>
#include <fairmq/Tools.h>
#include <FairMQLogger.h>

#include <zmq.h>

namespace fair
{
namespace mq
{
namespace ofi
{

using namespace std;

Socket::Socket(const TransportFactory& factory, const string& type, const string& name, const string& id /*= ""*/)
    : fId{id + "." + name + "." + type}
    , fBytesTx{0}
    , fBytesRx{0}
    , fMessagesTx{0}
    , fMessagesRx{0}
    , fSndTimeout{100}
    , fRcvTimeout{100}
{
    if (type != "pair") {
        throw SocketError{tools::ToString("Socket type '", type, "' not implemented for ofi transport.")};
    } else {
        fMetaSocket = zmq_socket(factory.fZmqContext, GetConstant(type));
    }

    if (fMetaSocket == nullptr) {
        throw SocketError{tools::ToString("Failed creating socket ", fId, ", reason: ", zmq_strerror(errno))};
    }

    if (zmq_setsockopt(fMetaSocket, ZMQ_IDENTITY, fId.c_str(), fId.length()) != 0) {
        throw SocketError{tools::ToString("Failed setting ZMQ_IDENTITY socket option, reason: ", zmq_strerror(errno))};
    }

    // Tell socket to try and send/receive outstanding messages for <linger> milliseconds before terminating.
    // Default value for ZeroMQ is -1, which is to wait forever.
    int linger = 1000;
    if (zmq_setsockopt(fMetaSocket, ZMQ_LINGER, &linger, sizeof(linger)) != 0) {
        throw SocketError{tools::ToString("Failed setting ZMQ_LINGER socket option, reason: ", zmq_strerror(errno))};
    }

    if (zmq_setsockopt(fMetaSocket, ZMQ_SNDTIMEO, &fSndTimeout, sizeof(fSndTimeout)) != 0) {
        throw SocketError{tools::ToString("Failed setting ZMQ_SNDTIMEO socket option, reason: ", zmq_strerror(errno))};
    }

    if (zmq_setsockopt(fMetaSocket, ZMQ_RCVTIMEO, &fRcvTimeout, sizeof(fRcvTimeout)) != 0) {
        throw SocketError{tools::ToString("Failed setting ZMQ_RCVTIMEO socket option, reason: ", zmq_strerror(errno))};
    }
}

auto Socket::Bind(const string& address) -> bool
{
    if (zmq_bind(fMetaSocket, address.c_str()) != 0) {
        if (errno == EADDRINUSE) {
            // do not print error in this case, this is handled by FairMQDevice
            // in case no connection could be established after trying a number of random ports from a range.
            return false;
        }
        LOG(error) << "Failed binding socket " << fId << ", reason: " << zmq_strerror(errno);
        return false;
    }
    return true;
}

auto Socket::Connect(const string& address) -> void
{
    if (zmq_connect(fMetaSocket, address.c_str()) != 0) {
        throw SocketError{tools::ToString("Failed connecting socket ", fId, ", reason: ", zmq_strerror(errno))};
    }
}

auto Socket::Send(FairMQMessagePtr& msg, const int timeout) -> int { return SendImpl(msg, 0, timeout); }
auto Socket::Receive(FairMQMessagePtr& msg, const int timeout) -> int { return ReceiveImpl(msg, 0, timeout); }
auto Socket::Send(std::vector<std::unique_ptr<FairMQMessage>>& msgVec, const int timeout) -> int64_t { return SendImpl(msgVec, 0, timeout); }
auto Socket::Receive(std::vector<std::unique_ptr<FairMQMessage>>& msgVec, const int timeout) -> int64_t { return ReceiveImpl(msgVec, 0, timeout); }

auto Socket::TrySend(FairMQMessagePtr& msg) -> int { return SendImpl(msg, ZMQ_DONTWAIT, 0); }
auto Socket::TryReceive(FairMQMessagePtr& msg) -> int { return ReceiveImpl(msg, ZMQ_DONTWAIT, 0); }
auto Socket::TrySend(std::vector<std::unique_ptr<FairMQMessage>>& msgVec) -> int64_t { return SendImpl(msgVec, ZMQ_DONTWAIT, 0); }
auto Socket::TryReceive(std::vector<std::unique_ptr<FairMQMessage>>& msgVec) -> int64_t { return ReceiveImpl(msgVec, ZMQ_DONTWAIT, 0); }

auto Socket::SendImpl(FairMQMessagePtr& msg, const int flags, const int timeout) -> int
{
    auto ret = zmq_send(fMetaSocket, nullptr, 0, flags);
    if (ret == EAGAIN) {
        return -2;
    } else if (ret < 0) {
        LOG(error) << "Failed sending meta message on socket " << fId << ", reason: " << zmq_strerror(errno);
        return -1;
    } else {
        return ret;
    }
}

auto Socket::ReceiveImpl(FairMQMessagePtr& msg, const int flags, const int timeout) -> int
{
    auto ret = zmq_recv(fMetaSocket, nullptr, 0, flags);
    if (ret == EAGAIN) {
        return -2;
    } else if (ret < 0) {
        LOG(error) << "Failed receiving meta message on socket " << fId << ", reason: " << zmq_strerror(errno);
        return -1;
    } else {
        return ret;
    }
}

auto Socket::SendImpl(vector<FairMQMessagePtr>& msgVec, const int flags, const int timeout) -> int64_t 
{
    throw SocketError{"Not yet implemented."};
    // const unsigned int vecSize = msgVec.size();
    // int elapsed = 0;
    //
    // // Sending vector typicaly handles more then one part
    // if (vecSize > 1)
    // {
    //     int64_t totalSize = 0;
    //     int nbytes = -1;
    //     bool repeat = false;
    //
    //     while (true && !fInterrupted)
    //     {
    //         for (unsigned int i = 0; i < vecSize; ++i)
    //         {
    //             nbytes = zmq_msg_send(static_cast<FairMQMessageSHM*>(msgVec[i].get())->GetMessage(),
    //                                   fSocket,
    //                                   (i < vecSize - 1) ? ZMQ_SNDMORE|flags : flags);
    //             if (nbytes >= 0)
    //             {
    //                 static_cast<FairMQMessageSHM*>(msgVec[i].get())->fQueued = true;
    //                 size_t size = msgVec[i]->GetSize();
    //
    //                 totalSize += size;
    //             }
    //             else
    //             {
    //                 // according to ZMQ docs, this can only occur for the first part
    //                 if (zmq_errno() == EAGAIN)
    //                 {
    //                     if (!fInterrupted && ((flags & ZMQ_DONTWAIT) == 0))
    //                     {
    //                         if (timeout)
    //                         {
    //                             elapsed += fSndTimeout;
    //                             if (elapsed >= timeout)
    //                             {
    //                                 return -2;
    //                             }
    //                         }
    //                         repeat = true;
    //                         break;
    //                     }
    //                     else
    //                     {
    //                         return -2;
    //                     }
    //                 }
    //                 if (zmq_errno() == ETERM)
    //                 {
    //                     LOG(info) << "terminating socket " << fId;
    //                     return -1;
    //                 }
    //                 LOG(error) << "Failed sending on socket " << fId << ", reason: " << zmq_strerror(errno);
    //                 return nbytes;
    //             }
    //         }
    //
    //         if (repeat)
    //         {
    //             continue;
    //         }
    //
    //         // store statistics on how many messages have been sent (handle all parts as a single message)
    //         ++fMessagesTx;
    //         fBytesTx += totalSize;
    //         return totalSize;
    //     }
    //
    //     return -1;
    // } // If there's only one part, send it as a regular message
    // else if (vecSize == 1)
    // {
    //     return Send(msgVec.back(), flags);
    // }
    // else // if the vector is empty, something might be wrong
    // {
    //     LOG(warn) << "Will not send empty vector";
    //     return -1;
    // }
}

auto Socket::ReceiveImpl(vector<FairMQMessagePtr>& msgVec, const int flags, const int timeout) -> int64_t
{
    throw SocketError{"Not yet implemented."};
    // int64_t totalSize = 0;
    // int64_t more = 0;
    // bool repeat = false;
    // int elapsed = 0;
    //
    // while (true)
    // {
    //     // Warn if the vector is filled before Receive() and empty it.
    //     // if (msgVec.size() > 0)
    //     // {
    //     //     LOG(warn) << "Message vector contains elements before Receive(), they will be deleted!";
    //     //     msgVec.clear();
    //     // }
    //
    //     totalSize = 0;
    //     more = 0;
    //     repeat = false;
    //
    //     do
    //     {
    //         FairMQMessagePtr part(new FairMQMessageSHM(fManager));
    //         zmq_msg_t* msgPtr = static_cast<FairMQMessageSHM*>(part.get())->GetMessage();
    //
    //         int nbytes = zmq_msg_recv(msgPtr, fSocket, flags);
    //         if (nbytes == 0)
    //         {
    //             msgVec.push_back(move(part));
    //         }
    //         else if (nbytes > 0)
    //         {
    //             MetaHeader* hdr = static_cast<MetaHeader*>(zmq_msg_data(msgPtr));
    //             size_t size = 0;
    //             static_cast<FairMQMessageSHM*>(part.get())->fHandle = hdr->fHandle;
    //             static_cast<FairMQMessageSHM*>(part.get())->fSize = hdr->fSize;
    //             static_cast<FairMQMessageSHM*>(part.get())->fRegionId = hdr->fRegionId;
    //             static_cast<FairMQMessageSHM*>(part.get())->fHint = hdr->fHint;
    //             size = part->GetSize();
    //
    //             msgVec.push_back(move(part));
    //
    //             totalSize += size;
    //         }
    //         else if (zmq_errno() == EAGAIN)
    //         {
    //             if (!fInterrupted && ((flags & ZMQ_DONTWAIT) == 0))
    //             {
    //                 if (timeout)
    //                 {
    //                     elapsed += fSndTimeout;
    //                     if (elapsed >= timeout)
    //                     {
    //                         return -2;
    //                     }
    //                 }
    //                 repeat = true;
    //                 break;
    //             }
    //             else
    //             {
    //                 return -2;
    //             }
    //         }
    //         else
    //         {
    //             return nbytes;
    //         }
    //
    //         size_t more_size = sizeof(more);
    //         zmq_getsockopt(fSocket, ZMQ_RCVMORE, &more, &more_size);
    //     }
    //     while (more);
    //
    //     if (repeat)
    //     {
    //         continue;
    //     }
    //
    //     // store statistics on how many messages have been received (handle all parts as a single message)
    //     ++fMessagesRx;
    //     fBytesRx += totalSize;
    //     return totalSize;
    // }
}

auto Socket::Close() -> void
{
    if (zmq_close(fMetaSocket) != 0) {
        throw SocketError{tools::ToString("Failed closing zmq socket, reason: ", zmq_strerror(errno))};
    }
}

auto Socket::SetOption(const string& option, const void* value, size_t valueSize) -> void
{
    if (zmq_setsockopt(fMetaSocket, GetConstant(option), value, valueSize) < 0) {
        throw SocketError{tools::ToString("Failed setting socket option, reason: ", zmq_strerror(errno))};
    }
}

auto Socket::GetOption(const string& option, void* value, size_t* valueSize) -> void
{
    if (zmq_getsockopt(fMetaSocket, GetConstant(option), value, valueSize) < 0) {
        throw SocketError{tools::ToString("Failed getting socket option, reason: ", zmq_strerror(errno))};
    }
}

auto Socket::SetSendTimeout(const int timeout, const string& address, const string& method) -> bool
{
    throw SocketError{"Not yet implemented."};
    // fSndTimeout = timeout;
    // if (method == "bind")
    // {
    //     if (zmq_unbind(fSocket, address.c_str()) != 0)
    //     {
    //         LOG(error) << "Failed unbinding socket " << fId << ", reason: " << zmq_strerror(errno);
    //         return false;
    //     }
    //     if (zmq_setsockopt(fSocket, ZMQ_SNDTIMEO, &fSndTimeout, sizeof(fSndTimeout)) != 0)
    //     {
    //         LOG(error) << "Failed setting option on socket " << fId << ", reason: " << zmq_strerror(errno);
    //         return false;
    //     }
    //     if (zmq_bind(fSocket, address.c_str()) != 0)
    //     {
    //         LOG(error) << "Failed binding socket " << fId << ", reason: " << zmq_strerror(errno);
    //         return false;
    //     }
    // }
    // else if (method == "connect")
    // {
    //     if (zmq_disconnect(fSocket, address.c_str()) != 0)
    //     {
    //         LOG(error) << "Failed disconnecting socket " << fId << ", reason: " << zmq_strerror(errno);
    //         return false;
    //     }
    //     if (zmq_setsockopt(fSocket, ZMQ_SNDTIMEO, &fSndTimeout, sizeof(fSndTimeout)) != 0)
    //     {
    //         LOG(error) << "Failed setting option on socket " << fId << ", reason: " << zmq_strerror(errno);
    //         return false;
    //     }
    //     if (zmq_connect(fSocket, address.c_str()) != 0)
    //     {
    //         LOG(error) << "Failed connecting socket " << fId << ", reason: " << zmq_strerror(errno);
    //         return false;
    //     }
    // }
    // else
    // {
    //     LOG(error) << "timeout failed - unknown method provided!";
    //     return false;
    // }
    //
    // return true;
}

auto Socket::GetSendTimeout() const -> int
{
    throw SocketError{"Not yet implemented."};
    // return fSndTimeout;
}

auto Socket::SetReceiveTimeout(const int timeout, const string& address, const string& method) -> bool
{
    throw SocketError{"Not yet implemented."};
    // fRcvTimeout = timeout;
    // if (method == "bind")
    // {
    //     if (zmq_unbind(fSocket, address.c_str()) != 0)
    //     {
    //         LOG(error) << "Failed unbinding socket " << fId << ", reason: " << zmq_strerror(errno);
    //         return false;
    //     }
    //     if (zmq_setsockopt(fSocket, ZMQ_RCVTIMEO, &fRcvTimeout, sizeof(fRcvTimeout)) != 0)
    //     {
    //         LOG(error) << "Failed setting option on socket " << fId << ", reason: " << zmq_strerror(errno);
    //         return false;
    //     }
    //     if (zmq_bind(fSocket, address.c_str()) != 0)
    //     {
    //         LOG(error) << "Failed binding socket " << fId << ", reason: " << zmq_strerror(errno);
    //         return false;
    //     }
    // }
    // else if (method == "connect")
    // {
    //     if (zmq_disconnect(fSocket, address.c_str()) != 0)
    //     {
    //         LOG(error) << "Failed disconnecting socket " << fId << ", reason: " << zmq_strerror(errno);
    //         return false;
    //     }
    //     if (zmq_setsockopt(fSocket, ZMQ_RCVTIMEO, &fRcvTimeout, sizeof(fRcvTimeout)) != 0)
    //     {
    //         LOG(error) << "Failed setting option on socket " << fId << ", reason: " << zmq_strerror(errno);
    //         return false;
    //     }
    //     if (zmq_connect(fSocket, address.c_str()) != 0)
    //     {
    //         LOG(error) << "Failed connecting socket " << fId << ", reason: " << zmq_strerror(errno);
    //         return false;
    //     }
    // }
    // else
    // {
    //     LOG(error) << "timeout failed - unknown method provided!";
    //     return false;
    // }
    //
    // return true;
}

auto Socket::GetReceiveTimeout() const -> int
{
    throw SocketError{"Not yet implemented."};
    // return fRcvTimeout;
}

auto Socket::GetConstant(const string& constant) -> int
{
    if (constant == "")
        return 0;
    if (constant == "sub")
        return ZMQ_SUB;
    if (constant == "pub")
        return ZMQ_PUB;
    if (constant == "xsub")
        return ZMQ_XSUB;
    if (constant == "xpub")
        return ZMQ_XPUB;
    if (constant == "push")
        return ZMQ_PUSH;
    if (constant == "pull")
        return ZMQ_PULL;
    if (constant == "req")
        return ZMQ_REQ;
    if (constant == "rep")
        return ZMQ_REP;
    if (constant == "dealer")
        return ZMQ_DEALER;
    if (constant == "router")
        return ZMQ_ROUTER;
    if (constant == "pair")
        return ZMQ_PAIR;

    if (constant == "snd-hwm")
        return ZMQ_SNDHWM;
    if (constant == "rcv-hwm")
        return ZMQ_RCVHWM;
    if (constant == "snd-size")
        return ZMQ_SNDBUF;
    if (constant == "rcv-size")
        return ZMQ_RCVBUF;
    if (constant == "snd-more")
        return ZMQ_SNDMORE;
    if (constant == "rcv-more")
        return ZMQ_RCVMORE;

    if (constant == "linger")
        return ZMQ_LINGER;
    if (constant == "no-block")
        return ZMQ_DONTWAIT;
    if (constant == "snd-more no-block")
        return ZMQ_DONTWAIT|ZMQ_SNDMORE;

    return -1;
}

Socket::~Socket()
{
    Close(); // NOLINT(clang-analyzer-optin.cplusplus.VirtualCall)
}

} /* namespace ofi */
} /* namespace mq */
} /* namespace fair */
