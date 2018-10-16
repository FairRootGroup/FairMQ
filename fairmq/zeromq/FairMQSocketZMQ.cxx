/********************************************************************************
 * Copyright (C) 2014-2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "FairMQSocketZMQ.h"
#include "FairMQMessageZMQ.h"
#include "FairMQLogger.h"
#include <fairmq/Tools.h>

#include <zmq.h>

#include <cassert>

using namespace std;
using namespace fair::mq;

atomic<bool> FairMQSocketZMQ::fInterrupted(false);

FairMQSocketZMQ::FairMQSocketZMQ(const string& type, const string& name, const string& id /*= ""*/, void* context)
    : fSocket(nullptr)
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

    if (type == "sub")
    {
        if (zmq_setsockopt(fSocket, ZMQ_SUBSCRIBE, nullptr, 0) != 0)
        {
            LOG(error) << "Failed setting ZMQ_SUBSCRIBE socket option, reason: " << zmq_strerror(errno);
        }
    }

    // LOG(info) << "created socket " << fId;
}

string FairMQSocketZMQ::GetId()
{
    return fId;
}

bool FairMQSocketZMQ::Bind(const string& address)
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

void FairMQSocketZMQ::Connect(const string& address)
{
    // LOG(info) << "connect socket " << fId << " on " << address;

    if (zmq_connect(fSocket, address.c_str()) != 0)
    {
        LOG(error) << "Failed connecting socket " << fId << ", reason: " << zmq_strerror(errno);
        // error here means incorrect configuration. exit if it happens.
        exit(EXIT_FAILURE);
    }
}

int FairMQSocketZMQ::Send(FairMQMessagePtr& msg, const int timeout) { return SendImpl(msg, 0, timeout); }
int FairMQSocketZMQ::Receive(FairMQMessagePtr& msg, const int timeout) { return ReceiveImpl(msg, 0, timeout); }
int64_t FairMQSocketZMQ::Send(vector<unique_ptr<FairMQMessage>>& msgVec, const int timeout) { return SendImpl(msgVec, 0, timeout); }
int64_t FairMQSocketZMQ::Receive(vector<unique_ptr<FairMQMessage>>& msgVec, const int timeout) { return ReceiveImpl(msgVec, 0, timeout); }

int FairMQSocketZMQ::TrySend(FairMQMessagePtr& msg) { return SendImpl(msg, ZMQ_DONTWAIT, 0); }
int FairMQSocketZMQ::TryReceive(FairMQMessagePtr& msg) { return ReceiveImpl(msg, ZMQ_DONTWAIT, 0); }
int64_t FairMQSocketZMQ::TrySend(vector<unique_ptr<FairMQMessage>>& msgVec) { return SendImpl(msgVec, ZMQ_DONTWAIT, 0); }
int64_t FairMQSocketZMQ::TryReceive(vector<unique_ptr<FairMQMessage>>& msgVec) { return ReceiveImpl(msgVec, ZMQ_DONTWAIT, 0); }

int FairMQSocketZMQ::SendImpl(FairMQMessagePtr& msg, const int flags, const int timeout)
{
    int elapsed = 0;

    static_cast<FairMQMessageZMQ*>(msg.get())->ApplyUsedSize();

    while (true)
    {
        int nbytes = zmq_msg_send(static_cast<FairMQMessageZMQ*>(msg.get())->GetMessage(), fSocket, flags);
        if (nbytes >= 0)
        {
            fBytesTx += nbytes;
            ++fMessagesTx;

            return nbytes;
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
}

int FairMQSocketZMQ::ReceiveImpl(FairMQMessagePtr& msg, const int flags, const int timeout)
{
    int elapsed = 0;

    while (true)
    {
        int nbytes = zmq_msg_recv(static_cast<FairMQMessageZMQ*>(msg.get())->GetMessage(), fSocket, flags);
        if (nbytes >= 0)
        {
            fBytesRx += nbytes;
            ++fMessagesRx;
            return nbytes;
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

int64_t FairMQSocketZMQ::SendImpl(vector<FairMQMessagePtr>& msgVec, const int flags, const int timeout)
{
    const unsigned int vecSize = msgVec.size();

    // Sending vector typicaly handles more then one part
    if (vecSize > 1)
    {
        int elapsed = 0;
        while (true)
        {
            int64_t totalSize = 0;
            bool repeat = false;

            for (unsigned int i = 0; i < vecSize; ++i)
            {
                static_cast<FairMQMessageZMQ*>(msgVec[i].get())->ApplyUsedSize();

                int nbytes = zmq_msg_send(static_cast<FairMQMessageZMQ*>(msgVec[i].get())->GetMessage(),
                                      fSocket,
                                      (i < vecSize - 1) ? ZMQ_SNDMORE|flags : flags);
                if (nbytes >= 0)
                {
                    totalSize += nbytes;
                }
                else
                {
                    // according to ZMQ docs, this can only occur for the first part
                    if (zmq_errno() == EAGAIN)
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
                            repeat = true;
                            break;
                        }
                        else
                        {
                            return -2;
                        }
                    }
                    if (zmq_errno() == ETERM)
                    {
                        LOG(info) << "terminating socket " << fId;
                        return -1;
                    }
                    LOG(error) << "Failed sending on socket " << fId << ", reason: " << zmq_strerror(errno);
                    return nbytes;
                }
            }

            if (repeat)
            {
                continue;
            }

            // store statistics on how many messages have been sent (handle all parts as a single message)
            ++fMessagesTx;
            fBytesTx += totalSize;
            return totalSize;
        }
    } // If there's only one part, send it as a regular message
    else if (vecSize == 1)
    {
        return SendImpl(msgVec.back(), flags, timeout);
    }
    else // if the vector is empty, something might be wrong
    {
        LOG(warn) << "Will not send empty vector";
        return -1;
    }
}

int64_t FairMQSocketZMQ::ReceiveImpl(vector<FairMQMessagePtr>& msgVec, const int flags, const int timeout)
{
    int elapsed = 0;

    while (true)
    {
        int64_t totalSize = 0;
        int64_t more = 0;
        bool repeat = false;

        do
        {
            unique_ptr<FairMQMessage> part(new FairMQMessageZMQ());

            int nbytes = zmq_msg_recv(static_cast<FairMQMessageZMQ*>(part.get())->GetMessage(), fSocket, flags);
            if (nbytes >= 0)
            {
                msgVec.push_back(move(part));
                totalSize += nbytes;
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
                    repeat = true;
                    break;
                }
                else
                {
                    return -2;
                }
            }
            else
            {
                return nbytes;
            }

            size_t moreSize = sizeof(more);
            zmq_getsockopt(fSocket, ZMQ_RCVMORE, &more, &moreSize);
        }
        while (more);

        if (repeat)
        {
            continue;
        }

        // store statistics on how many messages have been received (handle all parts as a single message)
        ++fMessagesRx;
        fBytesRx += totalSize;
        return totalSize;
    }
}

void FairMQSocketZMQ::Close()
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

void FairMQSocketZMQ::Interrupt()
{
    fInterrupted = true;
}

void FairMQSocketZMQ::Resume()
{
    fInterrupted = false;
}

void* FairMQSocketZMQ::GetSocket() const
{
    return fSocket;
}

void FairMQSocketZMQ::SetOption(const string& option, const void* value, size_t valueSize)
{
    if (zmq_setsockopt(fSocket, GetConstant(option), value, valueSize) < 0)
    {
        LOG(error) << "Failed setting socket option, reason: " << zmq_strerror(errno);
    }
}

void FairMQSocketZMQ::GetOption(const string& option, void* value, size_t* valueSize)
{
    if (zmq_getsockopt(fSocket, GetConstant(option), value, valueSize) < 0)
    {
        LOG(error) << "Failed getting socket option, reason: " << zmq_strerror(errno);
    }
}

void FairMQSocketZMQ::SetLinger(const int value)
{
    if (zmq_setsockopt(fSocket, ZMQ_LINGER, &value, sizeof(value)) < 0) {
        throw SocketError(tools::ToString("failed setting ZMQ_LINGER, reason: ", zmq_strerror(errno)));
    }
}

int FairMQSocketZMQ::GetLinger() const
{
    int value = 0;
    size_t valueSize = sizeof(value);
    if (zmq_getsockopt(fSocket, ZMQ_LINGER, &value, &valueSize) < 0) {
        throw SocketError(tools::ToString("failed getting ZMQ_LINGER, reason: ", zmq_strerror(errno)));
    }
    return value;
}

void FairMQSocketZMQ::SetSndBufSize(const int value)
{
    if (zmq_setsockopt(fSocket, ZMQ_SNDHWM, &value, sizeof(value)) < 0) {
        throw SocketError(tools::ToString("failed setting ZMQ_SNDHWM, reason: ", zmq_strerror(errno)));
    }
}

int FairMQSocketZMQ::GetSndBufSize() const
{
    int value = 0;
    size_t valueSize = sizeof(value);
    if (zmq_getsockopt(fSocket, ZMQ_SNDHWM, &value, &valueSize) < 0) {
        throw SocketError(tools::ToString("failed getting ZMQ_SNDHWM, reason: ", zmq_strerror(errno)));
    }
    return value;
}

void FairMQSocketZMQ::SetRcvBufSize(const int value)
{
    if (zmq_setsockopt(fSocket, ZMQ_RCVHWM, &value, sizeof(value)) < 0) {
        throw SocketError(tools::ToString("failed setting ZMQ_RCVHWM, reason: ", zmq_strerror(errno)));
    }
}

int FairMQSocketZMQ::GetRcvBufSize() const
{
    int value = 0;
    size_t valueSize = sizeof(value);
    if (zmq_getsockopt(fSocket, ZMQ_RCVHWM, &value, &valueSize) < 0) {
        throw SocketError(tools::ToString("failed getting ZMQ_RCVHWM, reason: ", zmq_strerror(errno)));
    }
    return value;
}

void FairMQSocketZMQ::SetSndKernelSize(const int value)
{
    if (zmq_setsockopt(fSocket, ZMQ_SNDBUF, &value, sizeof(value)) < 0) {
        throw SocketError(tools::ToString("failed getting ZMQ_SNDBUF, reason: ", zmq_strerror(errno)));
    }
}

int FairMQSocketZMQ::GetSndKernelSize() const
{
    int value = 0;
    size_t valueSize = sizeof(value);
    if (zmq_getsockopt(fSocket, ZMQ_SNDBUF, &value, &valueSize) < 0) {
        throw SocketError(tools::ToString("failed getting ZMQ_SNDBUF, reason: ", zmq_strerror(errno)));
    }
    return value;
}

void FairMQSocketZMQ::SetRcvKernelSize(const int value)
{
    if (zmq_setsockopt(fSocket, ZMQ_RCVBUF, &value, sizeof(value)) < 0) {
        throw SocketError(tools::ToString("failed getting ZMQ_RCVBUF, reason: ", zmq_strerror(errno)));
    }
}

int FairMQSocketZMQ::GetRcvKernelSize() const
{
    int value = 0;
    size_t valueSize = sizeof(value);
    if (zmq_getsockopt(fSocket, ZMQ_RCVBUF, &value, &valueSize) < 0) {
        throw SocketError(tools::ToString("failed getting ZMQ_RCVBUF, reason: ", zmq_strerror(errno)));
    }
    return value;
}

unsigned long FairMQSocketZMQ::GetBytesTx() const
{
    return fBytesTx;
}

unsigned long FairMQSocketZMQ::GetBytesRx() const
{
    return fBytesRx;
}

unsigned long FairMQSocketZMQ::GetMessagesTx() const
{
    return fMessagesTx;
}

unsigned long FairMQSocketZMQ::GetMessagesRx() const
{
    return fMessagesRx;
}

int FairMQSocketZMQ::GetConstant(const string& constant)
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

    return -1;
}

FairMQSocketZMQ::~FairMQSocketZMQ()
{
    Close();
}
