/********************************************************************************
 * Copyright (C) 2014-2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
#include <fairmq/shmem/Common.h>

#include "FairMQSocketSHM.h"
#include "FairMQMessageSHM.h"
#include "FairMQUnmanagedRegionSHM.h"
#include "FairMQLogger.h"

#include <zmq.h>

#include <stdexcept>

using namespace std;
using namespace fair::mq::shmem;

atomic<bool> FairMQSocketSHM::fInterrupted(false);

FairMQSocketSHM::FairMQSocketSHM(Manager& manager, const string& type, const string& name, const string& id /*= ""*/, void* context)
    : fSocket(nullptr)
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

    if (fSocket == nullptr)
    {
        LOG(error) << "Failed creating socket " << fId << ", reason: " << zmq_strerror(errno);
        exit(EXIT_FAILURE);
    }

    if (zmq_setsockopt(fSocket, ZMQ_IDENTITY, fId.c_str(), fId.length()) != 0)
    {
        LOG(error) << "Failed setting ZMQ_IDENTITY socket option, reason: " << zmq_strerror(errno);
    }

    // Tell socket to try and send/receive outstanding messages for <linger> milliseconds before terminating.
    // Default value for ZeroMQ is -1, which is to wait forever.
    int linger = 1000;
    if (zmq_setsockopt(fSocket, ZMQ_LINGER, &linger, sizeof(linger)) != 0)
    {
        LOG(error) << "Failed setting ZMQ_LINGER socket option, reason: " << zmq_strerror(errno);
    }

    if (zmq_setsockopt(fSocket, ZMQ_SNDTIMEO, &fSndTimeout, sizeof(fSndTimeout)) != 0)
    {
        LOG(error) << "Failed setting ZMQ_SNDTIMEO socket option, reason: " << zmq_strerror(errno);
    }

    if (zmq_setsockopt(fSocket, ZMQ_RCVTIMEO, &fRcvTimeout, sizeof(fRcvTimeout)) != 0)
    {
        LOG(error) << "Failed setting ZMQ_RCVTIMEO socket option, reason: " << zmq_strerror(errno);
    }

    // if (type == "sub")
    // {
    //     if (zmq_setsockopt(fSocket, ZMQ_SUBSCRIBE, nullptr, 0) != 0)
    //     {
    //         LOG(error) << "Failed setting ZMQ_SUBSCRIBE socket option, reason: " << zmq_strerror(errno);
    //     }
    // }

    if (type == "sub" || type == "pub")
    {
        LOG(error) << "PUB/SUB socket type is not supported for shared memory transport";
        throw fair::mq::SocketError("PUB/SUB socket type is not supported for shared memory transport");
    }

    // LOG(info) << "created socket " << fId;
}

bool FairMQSocketSHM::Bind(const string& address)
{
    // LOG(info) << "bind socket " << fId << " on " << address;

    if (zmq_bind(fSocket, address.c_str()) != 0)
    {
        if (errno == EADDRINUSE) {
            // do not print error in this case, this is handled by FairMQDevice in case no connection could be established after trying a number of random ports from a range.
            return false;
        }
        LOG(error) << "Failed binding socket " << fId << ", reason: " << zmq_strerror(errno);
        return false;
    }
    return true;
}

void FairMQSocketSHM::Connect(const string& address)
{
    // LOG(info) << "connect socket " << fId << " on " << address;

    if (zmq_connect(fSocket, address.c_str()) != 0)
    {
        LOG(error) << "Failed connecting socket " << fId << ", reason: " << zmq_strerror(errno);
        // error here means incorrect configuration. exit if it happens.
        exit(EXIT_FAILURE);
    }
}

int FairMQSocketSHM::Send(FairMQMessagePtr& msg, const int timeout) { return SendImpl(msg, 0, timeout); }
int FairMQSocketSHM::Receive(FairMQMessagePtr& msg, const int timeout) { return ReceiveImpl(msg, 0, timeout); }
int64_t FairMQSocketSHM::Send(vector<unique_ptr<FairMQMessage>>& msgVec, const int timeout) { return SendImpl(msgVec, 0, timeout); }
int64_t FairMQSocketSHM::Receive(vector<unique_ptr<FairMQMessage>>& msgVec, const int timeout) { return ReceiveImpl(msgVec, 0, timeout); }

int FairMQSocketSHM::TrySend(FairMQMessagePtr& msg) { return SendImpl(msg, ZMQ_DONTWAIT, 0); }
int FairMQSocketSHM::TryReceive(FairMQMessagePtr& msg) { return ReceiveImpl(msg, ZMQ_DONTWAIT, 0); }
int64_t FairMQSocketSHM::TrySend(vector<unique_ptr<FairMQMessage>>& msgVec) { return SendImpl(msgVec, ZMQ_DONTWAIT, 0); }
int64_t FairMQSocketSHM::TryReceive(vector<unique_ptr<FairMQMessage>>& msgVec) { return ReceiveImpl(msgVec, ZMQ_DONTWAIT, 0); }

int FairMQSocketSHM::SendImpl(FairMQMessagePtr& msg, const int flags, const int timeout)
{
    int nbytes = -1;
    int elapsed = 0;

    while (true && !fInterrupted)
    {
        nbytes = zmq_msg_send(static_cast<FairMQMessageSHM*>(msg.get())->GetMessage(), fSocket, flags);
        if (nbytes == 0)
        {
            return nbytes;
        }
        else if (nbytes > 0)
        {
            static_cast<FairMQMessageSHM*>(msg.get())->fQueued = true;

            size_t size = msg->GetSize();
            fBytesTx += size;
            ++fMessagesTx;

            return size;
        }
        else if (zmq_errno() == EAGAIN)
        {
            if (!fInterrupted && ((flags & ZMQ_DONTWAIT) == 0))
            {
                if (timeout)
                {
                    elapsed += fSndTimeout;
                    if (elapsed >= timeout)
                    {
                        return -2;
                    }
                }
                continue;
            }
            else
            {
                return -2;
            }
        }
        else if (zmq_errno() == ETERM)
        {
            LOG(info) << "terminating socket " << fId;
            return -1;
        }
        else
        {
            LOG(error) << "Failed sending on socket " << fId << ", reason: " << zmq_strerror(errno);
            return nbytes;
        }
    }

    return -1;
}

int FairMQSocketSHM::ReceiveImpl(FairMQMessagePtr& msg, const int flags, const int timeout)
{
    int nbytes = -1;
    int elapsed = 0;

    zmq_msg_t* msgPtr = static_cast<FairMQMessageSHM*>(msg.get())->GetMessage();
    while (true)
    {
        nbytes = zmq_msg_recv(msgPtr, fSocket, flags);
        if (nbytes == 0)
        {
            ++fMessagesRx;

            return nbytes;
        }
        else if (nbytes > 0)
        {
            // check for number of receiving messages. must be 1
            const auto numMsgs = nbytes / sizeof(MetaHeader);
            if (numMsgs > 1)
            {
                LOG(error) << "Receiving SHM multipart with a single message receive call";
            }

            assert (numMsgs == 1);

            MetaHeader* hdr = static_cast<MetaHeader*>(zmq_msg_data(msgPtr));
            size_t size = 0;
            static_cast<FairMQMessageSHM*>(msg.get())->fHandle = hdr->fHandle;
            static_cast<FairMQMessageSHM*>(msg.get())->fSize = hdr->fSize;
            static_cast<FairMQMessageSHM*>(msg.get())->fRegionId = hdr->fRegionId;
            static_cast<FairMQMessageSHM*>(msg.get())->fHint = hdr->fHint;
            size = msg->GetSize();

            fBytesRx += size;
            ++fMessagesRx;

            return size;
        }
        else if (zmq_errno() == EAGAIN)
        {
            if (!fInterrupted && ((flags & ZMQ_DONTWAIT) == 0))
            {
                if (timeout)
                {
                    elapsed += fRcvTimeout;
                    if (elapsed >= timeout)
                    {
                        return -2;
                    }
                }
                continue;
            }
            else
            {
                return -2;
            }
        }
        else if (zmq_errno() == ETERM)
        {
            LOG(info) << "terminating socket " << fId;
            return -1;
        }
        else
        {
            LOG(error) << "Failed receiving on socket " << fId << ", reason: " << zmq_strerror(errno);
            return nbytes;
        }
    }
}

int64_t FairMQSocketSHM::SendImpl(vector<FairMQMessagePtr>& msgVec, const int flags, const int timeout)
{
    const unsigned int vecSize = msgVec.size();
    int64_t totalSize = 0;
    int elapsed = 0;

    if (vecSize == 1) {
        return SendImpl(msgVec.back(), flags, timeout);
    }

    // put it into zmq message
    zmq_msg_t zmqMsg;
    zmq_msg_init_size(&zmqMsg, vecSize * sizeof(MetaHeader));

    // prepare the message with shm metas
    MetaHeader* metas = static_cast<MetaHeader*>(zmq_msg_data(&zmqMsg));

    for (auto &msg : msgVec)
    {
        zmq_msg_t* metaMsg = static_cast<FairMQMessageSHM*>(msg.get())->GetMessage();
        memcpy(metas++, zmq_msg_data(metaMsg), sizeof(MetaHeader));
    }

    while (!fInterrupted)
    {
        int nbytes = -1;
        nbytes = zmq_msg_send(&zmqMsg, fSocket, flags);

        if (nbytes == 0)
        {
            zmq_msg_close(&zmqMsg);
            return nbytes;
        }
        else if (nbytes > 0)
        {
            assert(nbytes == (vecSize * sizeof(MetaHeader))); // all or nothing

            for (auto &msg : msgVec)
            {
                FairMQMessageSHM* shmMsg = static_cast<FairMQMessageSHM*>(msg.get());
                shmMsg->fQueued = true;
                totalSize += shmMsg->fSize;
            }

            // store statistics on how many messages have been sent
            fMessagesTx++;
            fBytesTx += totalSize;

            zmq_msg_close(&zmqMsg);
            return totalSize;
        }
        else if (zmq_errno() == EAGAIN)
        {
            if (!fInterrupted && ((flags & ZMQ_DONTWAIT) == 0))
            {
                if (timeout)
                {
                    elapsed += fSndTimeout;
                    if (elapsed >= timeout)
                    {
                        zmq_msg_close(&zmqMsg);
                        return -2;
                    }
                }
                continue;
            }
            else
            {
                zmq_msg_close(&zmqMsg);
                return -2;
            }
        }
        else if (zmq_errno() == ETERM)
        {
            zmq_msg_close(&zmqMsg);
            LOG(info) << "terminating socket " << fId;
            return -1;
        }
        else
        {
            zmq_msg_close(&zmqMsg);
            LOG(error) << "Failed sending on socket " << fId << ", reason: " << zmq_strerror(errno);
            return nbytes;
        }
    }

    zmq_msg_close(&zmqMsg);
    return -1;
}

int64_t FairMQSocketSHM::ReceiveImpl(vector<FairMQMessagePtr>& msgVec, const int flags, const int timeout)
{
    int64_t totalSize = 0;
    int elapsed = 0;

    zmq_msg_t zmqMsg;
    zmq_msg_init(&zmqMsg);

    while (!fInterrupted)
    {
        int nbytes = zmq_msg_recv(&zmqMsg, fSocket, flags);
        if (nbytes == 0)
        {
            zmq_msg_close(&zmqMsg);
            return 0;
        }
        else if (nbytes > 0)
        {
            MetaHeader* hdrVec = static_cast<MetaHeader*>(zmq_msg_data(&zmqMsg));
            const auto hdrVecSize = zmq_msg_size(&zmqMsg);
            assert(hdrVecSize > 0);
            assert(hdrVecSize % sizeof(MetaHeader) == 0);

            const auto numMessages = hdrVecSize / sizeof(MetaHeader);

            msgVec.reserve(numMessages);

            for (size_t m = 0; m < numMessages; m++)
            {
                MetaHeader metaHeader;
                memcpy(&metaHeader, &hdrVec[m], sizeof(MetaHeader));

                msgVec.emplace_back(fair::mq::tools::make_unique<FairMQMessageSHM>(fManager));

                FairMQMessageSHM* msg = static_cast<FairMQMessageSHM*>(msgVec.back().get());
                MetaHeader* msgHdr = static_cast<MetaHeader*>(zmq_msg_data(msg->GetMessage()));

                memcpy(msgHdr, &metaHeader, sizeof(MetaHeader));

                msg->fHandle = metaHeader.fHandle;
                msg->fSize = metaHeader.fSize;
                msg->fRegionId = metaHeader.fRegionId;
                msg->fHint = metaHeader.fHint;

                totalSize += msg->GetSize();
            }

            // store statistics on how many messages have been received (handle all parts as a single message)
            fMessagesRx++;
            fBytesRx += totalSize;

            zmq_msg_close(&zmqMsg);
            return totalSize;
        }
        else if (zmq_errno() == EAGAIN)
        {
            if (!fInterrupted && ((flags & ZMQ_DONTWAIT) == 0))
            {
                if (timeout)
                {
                    elapsed += fRcvTimeout;
                    if (elapsed >= timeout)
                    {
                        zmq_msg_close(&zmqMsg);
                        return -2;
                    }
                }
                continue;
            }
            else
            {
                zmq_msg_close(&zmqMsg);
                return -2;
            }
        }
        else
        {
            zmq_msg_close(&zmqMsg);
            LOG(error) << "Failed receiving on socket " << fId << ", reason: " << zmq_strerror(errno);
            return nbytes;
        }
    }

    zmq_msg_close(&zmqMsg);
    return -1;
}

void FairMQSocketSHM::Close()
{
    // LOG(debug) << "Closing socket " << fId;

    if (fSocket == nullptr)
    {
        return;
    }

    if (zmq_close(fSocket) != 0)
    {
        LOG(error) << "Failed closing socket " << fId << ", reason: " << zmq_strerror(errno);
    }

    fSocket = nullptr;
}

void FairMQSocketSHM::Interrupt()
{
    Manager::Interrupt();
    FairMQMessageSHM::fInterrupted = true;
    fInterrupted = true;
}

void FairMQSocketSHM::Resume()
{
    Manager::Resume();
    FairMQMessageSHM::fInterrupted = false;
    fInterrupted = false;
}

void* FairMQSocketSHM::GetSocket() const
{
    return fSocket;
}

int FairMQSocketSHM::GetSocket(int) const
{
    // dummy method to comply with the interface. functionality not possible in zeromq.
    return -1;
}

void FairMQSocketSHM::SetOption(const string& option, const void* value, size_t valueSize)
{
    if (zmq_setsockopt(fSocket, GetConstant(option), value, valueSize) < 0)
    {
        LOG(error) << "Failed setting socket option, reason: " << zmq_strerror(errno);
    }
}

void FairMQSocketSHM::GetOption(const string& option, void* value, size_t* valueSize)
{
    if (zmq_getsockopt(fSocket, GetConstant(option), value, valueSize) < 0)
    {
        LOG(error) << "Failed getting socket option, reason: " << zmq_strerror(errno);
    }
}

unsigned long FairMQSocketSHM::GetBytesTx() const
{
    return fBytesTx;
}

unsigned long FairMQSocketSHM::GetBytesRx() const
{
    return fBytesRx;
}

unsigned long FairMQSocketSHM::GetMessagesTx() const
{
    return fMessagesTx;
}

unsigned long FairMQSocketSHM::GetMessagesRx() const
{
    return fMessagesRx;
}

bool FairMQSocketSHM::SetSendTimeout(const int timeout, const string& address, const string& method)
{
    fSndTimeout = timeout;
    if (method == "bind")
    {
        if (zmq_unbind(fSocket, address.c_str()) != 0)
        {
            LOG(error) << "Failed unbinding socket " << fId << ", reason: " << zmq_strerror(errno);
            return false;
        }
        if (zmq_setsockopt(fSocket, ZMQ_SNDTIMEO, &fSndTimeout, sizeof(fSndTimeout)) != 0)
        {
            LOG(error) << "Failed setting option on socket " << fId << ", reason: " << zmq_strerror(errno);
            return false;
        }
        if (zmq_bind(fSocket, address.c_str()) != 0)
        {
            LOG(error) << "Failed binding socket " << fId << ", reason: " << zmq_strerror(errno);
            return false;
        }
    }
    else if (method == "connect")
    {
        if (zmq_disconnect(fSocket, address.c_str()) != 0)
        {
            LOG(error) << "Failed disconnecting socket " << fId << ", reason: " << zmq_strerror(errno);
            return false;
        }
        if (zmq_setsockopt(fSocket, ZMQ_SNDTIMEO, &fSndTimeout, sizeof(fSndTimeout)) != 0)
        {
            LOG(error) << "Failed setting option on socket " << fId << ", reason: " << zmq_strerror(errno);
            return false;
        }
        if (zmq_connect(fSocket, address.c_str()) != 0)
        {
            LOG(error) << "Failed connecting socket " << fId << ", reason: " << zmq_strerror(errno);
            return false;
        }
    }
    else
    {
        LOG(error) << "timeout failed - unknown method provided!";
        return false;
    }

    return true;
}

int FairMQSocketSHM::GetSendTimeout() const
{
    return fSndTimeout;
}

bool FairMQSocketSHM::SetReceiveTimeout(const int timeout, const string& address, const string& method)
{
    fRcvTimeout = timeout;
    if (method == "bind")
    {
        if (zmq_unbind(fSocket, address.c_str()) != 0)
        {
            LOG(error) << "Failed unbinding socket " << fId << ", reason: " << zmq_strerror(errno);
            return false;
        }
        if (zmq_setsockopt(fSocket, ZMQ_RCVTIMEO, &fRcvTimeout, sizeof(fRcvTimeout)) != 0)
        {
            LOG(error) << "Failed setting option on socket " << fId << ", reason: " << zmq_strerror(errno);
            return false;
        }
        if (zmq_bind(fSocket, address.c_str()) != 0)
        {
            LOG(error) << "Failed binding socket " << fId << ", reason: " << zmq_strerror(errno);
            return false;
        }
    }
    else if (method == "connect")
    {
        if (zmq_disconnect(fSocket, address.c_str()) != 0)
        {
            LOG(error) << "Failed disconnecting socket " << fId << ", reason: " << zmq_strerror(errno);
            return false;
        }
        if (zmq_setsockopt(fSocket, ZMQ_RCVTIMEO, &fRcvTimeout, sizeof(fRcvTimeout)) != 0)
        {
            LOG(error) << "Failed setting option on socket " << fId << ", reason: " << zmq_strerror(errno);
            return false;
        }
        if (zmq_connect(fSocket, address.c_str()) != 0)
        {
            LOG(error) << "Failed connecting socket " << fId << ", reason: " << zmq_strerror(errno);
            return false;
        }
    }
    else
    {
        LOG(error) << "timeout failed - unknown method provided!";
        return false;
    }

    return true;
}

int FairMQSocketSHM::GetReceiveTimeout() const
{
    return fRcvTimeout;
}

int FairMQSocketSHM::GetConstant(const string& constant)
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

FairMQSocketSHM::~FairMQSocketSHM()
{
    Close();
}
