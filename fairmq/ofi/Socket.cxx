/********************************************************************************
 *    Copyright (C) 2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <fairmq/ofi/ControlMessages.h>
#include <fairmq/ofi/Socket.h>
#include <fairmq/ofi/TransportFactory.h>
#include <fairmq/Tools.h>
#include <FairMQLogger.h>

#include <arpa/inet.h>
#include <asiofi.hpp>
#include <boost/asio/buffer.hpp>
#include <cstring>
#include <netinet/in.h>
#include <rdma/fabric.h>
#include <rdma/fi_endpoint.h>
#include <rdma/fi_cm.h>
#include <sstream>
#include <string.h>
#include <sys/socket.h>
#include <zmq.h>

namespace fair
{
namespace mq
{
namespace ofi
{

using namespace std;

Socket::Socket(Context& context, const string& type, const string& name, const string& id /*= ""*/)
    : fControlSocket(nullptr)
    // , fMonitorSocket(nullptr)
    , fPassiveDataEndpoint(nullptr)
    , fDataEndpoint(nullptr)
    , fId(id + "." + name + "." + type)
    , fBytesTx(0)
    , fBytesRx(0)
    , fMessagesTx(0)
    , fMessagesRx(0)
    , fContext(context)
    , fIoStrand(fContext.GetIoContext())
    , fSndTimeout(100)
    , fRcvTimeout(100)
{
    if (type != "pair") {
        throw SocketError{tools::ToString("Socket type '", type, "' not implemented for ofi transport.")};
    } else {
        fControlSocket = zmq_socket(fContext.GetZmqContext(), ZMQ_PAIR);

        if (fControlSocket == nullptr)
            throw SocketError{tools::ToString("Failed creating zmq meta socket ", fId, ", reason: ", zmq_strerror(errno))};

        if (zmq_setsockopt(fControlSocket, ZMQ_IDENTITY, fId.c_str(), fId.length()) != 0)
            throw SocketError{tools::ToString("Failed setting ZMQ_IDENTITY socket option, reason: ", zmq_strerror(errno))};

        // Tell socket to try and send/receive outstanding messages for <linger> milliseconds before terminating.
        // Default value for ZeroMQ is -1, which is to wait forever.
        int linger = 1000;
        if (zmq_setsockopt(fControlSocket, ZMQ_LINGER, &linger, sizeof(linger)) != 0)
            throw SocketError{tools::ToString("Failed setting ZMQ_LINGER socket option, reason: ", zmq_strerror(errno))};

        // TODO enable again and implement retries
        // if (zmq_setsockopt(fControlSocket, ZMQ_SNDTIMEO, &fSndTimeout, sizeof(fSndTimeout)) != 0)
        //     throw SocketError{tools::ToString("Failed setting ZMQ_SNDTIMEO socket option, reason: ", zmq_strerror(errno))};
        //
        // if (zmq_setsockopt(fControlSocket, ZMQ_RCVTIMEO, &fRcvTimeout, sizeof(fRcvTimeout)) != 0)
        //     throw SocketError{tools::ToString("Failed setting ZMQ_RCVTIMEO socket option, reason: ", zmq_strerror(errno))};

        // fMonitorSocket = zmq_socket(fContext.GetZmqContext(), ZMQ_PAIR);
        //
        // if (fMonitorSocket == nullptr)
        //     throw SocketError{tools::ToString("Failed creating zmq monitor socket ", fId, ", reason: ", zmq_strerror(errno))};
        //
        // auto mon_addr = tools::ToString("inproc://", fId);
        // if (zmq_socket_monitor(fControlSocket, mon_addr.c_str(), ZMQ_EVENT_ACCEPTED | ZMQ_EVENT_CONNECTED) < 0)
        //     throw SocketError{tools::ToString("Failed setting up monitor on meta socket, reason: ", zmq_strerror(errno))};
        //
        // if (zmq_connect(fMonitorSocket, mon_addr.c_str()) != 0)
        //     throw SocketError{tools::ToString("Failed connecting monitor socket to meta socket, reason: ", zmq_strerror(errno))};
    }
}

auto Socket::Bind(const string& address) -> bool
try {
    auto addr = Context::VerifyAddress(address);

    BindControlSocket(addr);

    // TODO make data port choice more robust
    addr.Port += 555;
    fLocalDataAddr = addr;
    BindDataEndpoint();

    AnnounceDataAddress();

    return true;
}
catch (const SilentSocketError& e)
{
    // do not print error in this case, this is handled by FairMQDevice
    // in case no connection could be established after trying a number of random ports from a range.
    return false;
}
catch (const SocketError& e)
{
    LOG(error) << "OFI transport: " << e.what();
    return false;
}

auto Socket::Connect(const string& address) -> bool
{
    auto addr = Context::VerifyAddress(address);

    ConnectControlSocket(addr);

    ProcessControlMessage(
        StaticUniquePtrDowncast<DataAddressAnnouncement>(ReceiveControlMessage()));

    ConnectDataEndpoint();
}

auto Socket::BindControlSocket(Context::Address address) -> void
{
    auto addr = tools::ToString("tcp://", address.Ip, ":", address.Port);

    if (zmq_bind(fControlSocket, addr.c_str()) != 0) {
        if (errno == EADDRINUSE) throw SilentSocketError("EADDRINUSE");
        throw SocketError(tools::ToString("Failed binding control socket ", fId, ", reason: ", zmq_strerror(errno)));
    }

    LOG(debug) << "OFI transport (" << fId << "): control band bound to " << address;
}

auto Socket::BindDataEndpoint() -> void
{
    assert(!fPassiveDataEndpoint);
    assert(!fDataEndpoint);

    fPassiveDataEndpoint = fContext.MakeOfiPassiveEndpoint(fLocalDataAddr);
    fPassiveDataEndpoint->listen([&](fid_t /*handle*/, asiofi::info&& info) {
        LOG(debug) << "OFI transport (" << fId << "): data band connection request received. Accepting ...";
        fDataEndpoint = fContext.MakeOfiConnectedEndpoint(info);
        fDataEndpoint->enable();
        fDataEndpoint->accept([&]() {
            LOG(debug) << "OFI transport (" << fId << "): data band connection accepted.";
        });
    });

    LOG(debug) << "OFI transport (" << fId << "): data band bound to " << fLocalDataAddr;
}

auto Socket::ConnectControlSocket(Context::Address address) -> void
{
    auto addr = tools::ToString("tcp://", address.Ip, ":", address.Port);

    if (zmq_connect(fControlSocket, addr.c_str()) != 0)
        throw SocketError(tools::ToString("Failed connecting control socket ", fId, ", reason: ", zmq_strerror(errno)));
}

auto Socket::ConnectDataEndpoint() -> void
{
    assert(!fDataEndpoint);

    fDataEndpoint = fContext.MakeOfiConnectedEndpoint(fRemoteDataAddr);
    fDataEndpoint->enable();

    LOG(debug) << "OFI transport (" << fId << "): local data band address: " << Context::ConvertAddress(fDataEndpoint->get_local_address());
    fDataEndpoint->connect([&]() {
        LOG(debug) << "OFI transport (" << fId << "): data band connected.";
    });
}

auto Socket::ProcessControlMessage(CtrlMsgPtr<DataAddressAnnouncement> daa) -> void
{
    assert(daa->type == ControlMessageType::DataAddressAnnouncement);

    sockaddr_in remoteAddr;
    remoteAddr.sin_family = AF_INET;
    remoteAddr.sin_port = daa->port;
    remoteAddr.sin_addr.s_addr = daa->ipv4;

    auto addr = Context::ConvertAddress(remoteAddr);
    LOG(debug) << "OFI transport (" << fId << "): Data address announcement of remote endpoint received: " << addr;
    fRemoteDataAddr = addr;
}

auto Socket::AnnounceDataAddress() -> void
try {
    // fLocalDataAddr = fDataEndpoint->get_local_address();
    // LOG(debug) << "Address of local ofi endpoint in socket " << fId << ": " << Context::ConvertAddress(fLocalDataAddr);

    // Create new data address announcement message
    auto daa = MakeControlMessage<DataAddressAnnouncement>(&fCtrlMemPool);
    auto addr = Context::ConvertAddress(fLocalDataAddr);
    daa->ipv4 = addr.sin_addr.s_addr;
    daa->port = addr.sin_port;

    SendControlMessage(StaticUniquePtrUpcast<ControlMessage>(std::move(daa)));

    LOG(debug) << "OFI transport (" << fId << "): data address announced.";
} catch (const SocketError& e) {
    throw SocketError(tools::ToString("Failed to announce data address, reason: ", e.what()));
}

auto Socket::SendControlMessage(CtrlMsgPtr<ControlMessage> ctrl) -> void
{
    assert(fControlSocket);
    // LOG(debug) << "About to send control message: " << ctrl->DebugString();

    // Serialize
    struct ZmqMsg
    {
        zmq_msg_t msg;
        ~ZmqMsg() { zmq_msg_close(&msg); }
        operator zmq_msg_t*() { return &msg; }
    } msg;

    switch (ctrl->type) {
        case ControlMessageType::DataAddressAnnouncement:
            {
                auto ret = zmq_msg_init_size(msg, sizeof(DataAddressAnnouncement));
                (void)ret;
                assert(ret == 0);
                std::memcpy(zmq_msg_data(msg), ctrl.get(), sizeof(DataAddressAnnouncement));
            }
            break;
        case ControlMessageType::PostBuffer:
            {
                auto ret = zmq_msg_init_size(msg, sizeof(PostBuffer));
                (void)ret;
                assert(ret == 0);
                std::memcpy(zmq_msg_data(msg), ctrl.get(), sizeof(PostBuffer));
            }
            break;
        default:
            throw SocketError(tools::ToString("Cannot send control message of unknown type."));
    }

    // Send
    if (zmq_msg_send(msg, fControlSocket, 0) == -1) {
        throw SocketError(
            tools::ToString("Failed to send control message, reason: ", zmq_strerror(errno)));
    }
}

auto Socket::ReceiveControlMessage() -> CtrlMsgPtr<ControlMessage>
{
    assert(fControlSocket);

    // Receive
    struct ZmqMsg
    {
        zmq_msg_t msg;
        ~ZmqMsg() { zmq_msg_close(&msg); }
        operator zmq_msg_t*() { return &msg; }
    } msg;
    auto ret = zmq_msg_init(msg);
    (void)ret;
    assert(ret == 0);
    if (zmq_msg_recv(msg, fControlSocket, 0) == -1) {
        throw SocketError(
            tools::ToString("Failed to receive control message, reason: ", zmq_strerror(errno)));
    }

    // Deserialize and sanity check
    const void* msg_data = zmq_msg_data(msg);
    const size_t msg_size = zmq_msg_size(msg);
    (void)msg_size;
    assert(msg_size >= sizeof(ControlMessage));

    switch (static_cast<const ControlMessage*>(msg_data)->type) {
        case ControlMessageType::DataAddressAnnouncement: {
            assert(msg_size == sizeof(DataAddressAnnouncement));
            auto daa = MakeControlMessage<DataAddressAnnouncement>(&fCtrlMemPool);
            std::memcpy(daa.get(), msg_data, sizeof(DataAddressAnnouncement));
            // LOG(debug) << "Received control message: " << ctrl->DebugString();
            return StaticUniquePtrUpcast<ControlMessage>(std::move(daa));
        }
        case ControlMessageType::PostBuffer: {
            assert(msg_size == sizeof(PostBuffer));
            auto pb = MakeControlMessage<PostBuffer>(&fCtrlMemPool);
            std::memcpy(pb.get(), msg_data, sizeof(PostBuffer));
            // LOG(debug) << "Received control message: " << ctrl->DebugString();
            return StaticUniquePtrUpcast<ControlMessage>(std::move(pb));
        }
        default:
            throw SocketError(tools::ToString("Received control message of unknown type."));
    }
}

// auto Socket::WaitForControlPeer() -> void
// {
    // assert(fWaitingForControlPeer);
//
    // First frame in message contains event number and value
    // zmq_msg_t msg;
    // zmq_msg_init(&msg);
    // if (zmq_msg_recv(&msg, fMonitorSocket, 0) == -1)
        // throw SocketError(tools::ToString("Failed to get monitor event, reason: ", zmq_strerror(errno)));
//
    // uint8_t* data = (uint8_t*) zmq_msg_data(&msg);
    // uint16_t event = *(uint16_t*)(data);
    // int value = *(uint32_t *)(data + 2);
//
    // Second frame in message contains event address
    // zmq_msg_init(&msg);
    // if (zmq_msg_recv(&msg, fMonitorSocket, 0) == -1)
        // throw SocketError(tools::ToString("Failed to get monitor event, reason: ", zmq_strerror(errno)));
//
    // if (event == ZMQ_EVENT_ACCEPTED) {
        // string localAddress = string(static_cast<char*>(zmq_msg_data(&msg)), zmq_msg_size(&msg));
        // sockaddr_in remoteAddr;
        // socklen_t addrSize = sizeof(sockaddr_in);
        // int ret = getpeername(value, (sockaddr*)&remoteAddr, &addrSize);
        // if (ret != 0)
            // throw SocketError(tools::ToString("Failed retrieving remote address, reason: ", strerror(errno)));
        // string remoteIp(inet_ntoa(remoteAddr.sin_addr));
        // int remotePort = ntohs(remoteAddr.sin_port);
        // LOG(debug) << "Accepted control peer connection from " << remoteIp << ":" << remotePort;
    // } else if (event == ZMQ_EVENT_CONNECTED) {
        // LOG(debug) << "Connected successfully to control peer";
    // } else {
        // LOG(debug) << "Unknown monitor event received: " << event << ". Ignoring.";
    // }
//
    // fWaitingForControlPeer = false;
// }

auto Socket::Send(MessagePtr& msg, const int timeout) -> int { return SendImpl(msg, 0, timeout); }
auto Socket::Receive(MessagePtr& msg, const int timeout) -> int { return ReceiveImpl(msg, 0, timeout); }
auto Socket::Send(std::vector<MessagePtr>& msgVec, const int timeout) -> int64_t { return SendImpl(msgVec, 0, timeout); }
auto Socket::Receive(std::vector<MessagePtr>& msgVec, const int timeout) -> int64_t { return ReceiveImpl(msgVec, 0, timeout); }

auto Socket::TrySend(MessagePtr& msg) -> int { return SendImpl(msg, ZMQ_DONTWAIT, 0); }
auto Socket::TryReceive(MessagePtr& msg) -> int { return ReceiveImpl(msg, ZMQ_DONTWAIT, 0); }
auto Socket::TrySend(std::vector<MessagePtr>& msgVec) -> int64_t { return SendImpl(msgVec, ZMQ_DONTWAIT, 0); }
auto Socket::TryReceive(std::vector<MessagePtr>& msgVec) -> int64_t { return ReceiveImpl(msgVec, ZMQ_DONTWAIT, 0); }

#include <mutex>
#include <condition_variable>

auto Socket::SendImpl(FairMQMessagePtr& msg, const int /*flags*/, const int /*timeout*/) -> int
try {
    auto size = msg->GetSize();
    LOG(debug) << "OFI transport (" << fId << "): ENTER SendImpl";

    // Create and send control message
    auto pb = MakeControlMessage<PostBuffer>(&fCtrlMemPool);
    pb->size = size;
    SendControlMessage(StaticUniquePtrUpcast<ControlMessage>(std::move(pb)));
    LOG(debug) << "OFI transport (" << fId << "): >>>>> SendImpl: Control message sent, size=" << size;

    if (size) {
        boost::asio::mutable_buffer buffer(msg->GetData(), size);
        asiofi::memory_region mr(fContext.GetDomain(), buffer, asiofi::mr::access::send);

        std::mutex m;
        std::condition_variable cv;
        bool completed(false);

        fDataEndpoint->send(
            buffer,
            mr.desc(),
            [&](boost::asio::mutable_buffer) {
                {
                    std::unique_lock<std::mutex> lk(m);
                    completed = true;
                }
                cv.notify_one();
                LOG(debug) << "OFI transport (" << fId << "):     > SendImpl: Data buffer sent";
            }
        );
        
        {
            std::unique_lock<std::mutex> lk(m);
            cv.wait(lk, [&](){ return completed; });
        }
        LOG(debug) << "OFI transport (" << fId << "): >>>>> SendImpl: Data send buffer posted";
    }

    msg.reset(nullptr);
    fBytesTx += size;
    fMessagesTx++;

    LOG(debug) << "OFI transport (" << fId << "): LEAVE SendImpl";
    return size;
}
catch (const SilentSocketError& e)
{
    return -2;
}
catch (const std::exception& e)
{
    LOG(error) << e.what();
    return -1;
}

auto Socket::ReceiveImpl(FairMQMessagePtr& msg, const int /*flags*/, const int /*timeout*/) -> int
try {
    LOG(debug) << "OFI transport (" << fId << "): ENTER ReceiveImpl";
    // Receive and process control message
    auto pb = StaticUniquePtrDowncast<PostBuffer>(ReceiveControlMessage());
    assert(pb.get());
    auto size = pb->size;
    LOG(debug) << "OFI transport (" << fId << "): <<<<< ReceiveImpl: Control message received, size=" << size;

    // Receive data
    if (size) {
        msg->Rebuild(size);
        boost::asio::mutable_buffer buffer(msg->GetData(), size);
        asiofi::memory_region mr(fContext.GetDomain(), buffer, asiofi::mr::access::recv);

        std::mutex m;
        std::condition_variable cv;
        bool completed(false);

        fDataEndpoint->recv(buffer, mr.desc(), [&](boost::asio::mutable_buffer) {
                {
                    std::unique_lock<std::mutex> lk(m);
                    completed = true;
                }
                cv.notify_one();
            }
        );
        LOG(debug) << "OFI transport (" << fId << "): <<<<< ReceiveImpl: Data buffer posted";

        {
            std::unique_lock<std::mutex> lk(m);
            cv.wait(lk, [&](){ return completed; });
        }
        LOG(debug) << "OFI transport (" << fId << "): <<<<< ReceiveImpl: Data received";
    }

    fBytesRx += size;
    fMessagesRx++;

    LOG(debug) << "OFI transport (" << fId << "): EXIT ReceiveImpl";
    return size;
}
catch (const SilentSocketError& e)
{
    return -2;
}
catch (const std::exception& e)
{
    LOG(error) << e.what();
    return -1;
}

auto Socket::SendImpl(vector<FairMQMessagePtr>& /*msgVec*/, const int /*flags*/, const int /*timeout*/) -> int64_t 
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

auto Socket::ReceiveImpl(vector<FairMQMessagePtr>& /*msgVec*/, const int /*flags*/, const int /*timeout*/) -> int64_t
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
    //         FairMQMessagePtr part(new FairMQMessageSHM(fManager, GetTransport()));
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
    if (zmq_close(fControlSocket) != 0)
        throw SocketError(tools::ToString("Failed closing zmq meta socket, reason: ", zmq_strerror(errno)));

    // if (zmq_close(fMonitorSocket) != 0)
        // throw SocketError(tools::ToString("Failed closing zmq monitor socket, reason: ", zmq_strerror(errno)));
}

auto Socket::SetOption(const string& option, const void* value, size_t valueSize) -> void
{
    if (zmq_setsockopt(fControlSocket, GetConstant(option), value, valueSize) < 0) {
        throw SocketError{tools::ToString("Failed setting socket option, reason: ", zmq_strerror(errno))};
    }
}

auto Socket::GetOption(const string& option, void* value, size_t* valueSize) -> void
{
    if (zmq_getsockopt(fControlSocket, GetConstant(option), value, valueSize) < 0) {
        throw SocketError{tools::ToString("Failed getting socket option, reason: ", zmq_strerror(errno))};
    }
}

int Socket::GetLinger() const
{
    int value = 0;
    size_t valueSize;
    if (zmq_getsockopt(fControlSocket, ZMQ_LINGER, &value, &valueSize) < 0) {
        throw SocketError(tools::ToString("failed getting ZMQ_LINGER, reason: ", zmq_strerror(errno)));
    }
    return value;
}

void Socket::SetSndBufSize(const int value)
{
    if (zmq_setsockopt(fControlSocket, ZMQ_SNDHWM, &value, sizeof(value)) < 0) {
        throw SocketError(tools::ToString("failed setting ZMQ_SNDHWM, reason: ", zmq_strerror(errno)));
    }
}

int Socket::GetSndBufSize() const
{
    int value = 0;
    size_t valueSize;
    if (zmq_getsockopt(fControlSocket, ZMQ_SNDHWM, &value, &valueSize) < 0) {
        throw SocketError(tools::ToString("failed getting ZMQ_SNDHWM, reason: ", zmq_strerror(errno)));
    }
    return value;
}

void Socket::SetRcvBufSize(const int value)
{
    if (zmq_setsockopt(fControlSocket, ZMQ_RCVHWM, &value, sizeof(value)) < 0) {
        throw SocketError(tools::ToString("failed setting ZMQ_RCVHWM, reason: ", zmq_strerror(errno)));
    }
}

int Socket::GetRcvBufSize() const
{
    int value = 0;
    size_t valueSize;
    if (zmq_getsockopt(fControlSocket, ZMQ_RCVHWM, &value, &valueSize) < 0) {
        throw SocketError(tools::ToString("failed getting ZMQ_RCVHWM, reason: ", zmq_strerror(errno)));
    }
    return value;
}

void Socket::SetSndKernelSize(const int value)
{
    if (zmq_setsockopt(fControlSocket, ZMQ_SNDBUF, &value, sizeof(value)) < 0) {
        throw SocketError(tools::ToString("failed getting ZMQ_SNDBUF, reason: ", zmq_strerror(errno)));
    }
}

int Socket::GetSndKernelSize() const
{
    int value = 0;
    size_t valueSize;
    if (zmq_getsockopt(fControlSocket, ZMQ_SNDBUF, &value, &valueSize) < 0) {
        throw SocketError(tools::ToString("failed getting ZMQ_SNDBUF, reason: ", zmq_strerror(errno)));
    }
    return value;
}

void Socket::SetRcvKernelSize(const int value)
{
    if (zmq_setsockopt(fControlSocket, ZMQ_RCVBUF, &value, sizeof(value)) < 0) {
        throw SocketError(tools::ToString("failed getting ZMQ_RCVBUF, reason: ", zmq_strerror(errno)));
    }
}

int Socket::GetRcvKernelSize() const
{
    int value = 0;
    size_t valueSize;
    if (zmq_getsockopt(fControlSocket, ZMQ_RCVBUF, &value, &valueSize) < 0) {
        throw SocketError(tools::ToString("failed getting ZMQ_RCVBUF, reason: ", zmq_strerror(errno)));
    }
    return value;
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
    try {
        Close(); // NOLINT(clang-analyzer-optin.cplusplus.VirtualCall)
    } catch (SocketError& e) {
        LOG(error) << e.what();
    }
}

} /* namespace ofi */
} /* namespace mq */
} /* namespace fair */
