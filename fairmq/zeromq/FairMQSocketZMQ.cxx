/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQSocketZMQ.cxx
 *
 * @since 2012-12-05
 * @author D. Klein, A. Rybalchenko
 */

#include <sstream>

#include <zmq.h>

#include "FairMQSocketZMQ.h"
#include "FairMQMessageZMQ.h"
#include "FairMQLogger.h"

using namespace std;

atomic<bool> FairMQSocketZMQ::fInterrupted(false);

FairMQSocketZMQ::FairMQSocketZMQ(const string& type, const string& name, const string& id /*= ""*/, void* context)
    : FairMQSocket(ZMQ_SNDMORE, ZMQ_RCVMORE, ZMQ_DONTWAIT)
    , fSocket(NULL)
    , fId()
    , fBytesTx(0)
    , fBytesRx(0)
    , fMessagesTx(0)
    , fMessagesRx(0)
{
    fId = id + "." + name + "." + type;

    assert(context);
    fSocket = zmq_socket(context, GetConstant(type));

    if (fSocket == NULL)
    {
        LOG(ERROR) << "Failed creating socket " << fId << ", reason: " << zmq_strerror(errno);
        exit(EXIT_FAILURE);
    }

    if (zmq_setsockopt(fSocket, ZMQ_IDENTITY, fId.c_str(), fId.length()) != 0)
    {
        LOG(ERROR) << "Failed setting ZMQ_IDENTITY socket option, reason: " << zmq_strerror(errno);
    }

    // Tell socket to try and send/receive outstanding messages for <linger> milliseconds before terminating.
    // Default value for ZeroMQ is -1, which is to wait forever.
    int linger = 1000;
    if (zmq_setsockopt(fSocket, ZMQ_LINGER, &linger, sizeof(linger)) != 0)
    {
        LOG(ERROR) << "Failed setting ZMQ_LINGER socket option, reason: " << zmq_strerror(errno);
    }

    int sndTimeout = 700;
    if (zmq_setsockopt(fSocket, ZMQ_SNDTIMEO, &sndTimeout, sizeof(sndTimeout)) != 0)
    {
        LOG(ERROR) << "Failed setting ZMQ_SNDTIMEO socket option, reason: " << zmq_strerror(errno);
    }

    int rcvTimeout = 700;
    if (zmq_setsockopt(fSocket, ZMQ_RCVTIMEO, &rcvTimeout, sizeof(rcvTimeout)) != 0)
    {
        LOG(ERROR) << "Failed setting ZMQ_RCVTIMEO socket option, reason: " << zmq_strerror(errno);
    }

    if (type == "sub")
    {
        if (zmq_setsockopt(fSocket, ZMQ_SUBSCRIBE, NULL, 0) != 0)
        {
            LOG(ERROR) << "Failed setting ZMQ_SUBSCRIBE socket option, reason: " << zmq_strerror(errno);
        }
    }

    // LOG(INFO) << "created socket " << fId;
}

string FairMQSocketZMQ::GetId()
{
    return fId;
}

bool FairMQSocketZMQ::Bind(const string& address)
{
    // LOG(INFO) << "bind socket " << fId << " on " << address;

    if (zmq_bind(fSocket, address.c_str()) != 0)
    {
        if (errno == EADDRINUSE) {
            // do not print error in this case, this is handled by FairMQDevice in case no connection could be established after trying a number of random ports from a range.
            return false;
        }
        LOG(ERROR) << "Failed binding socket " << fId << ", reason: " << zmq_strerror(errno);
        return false;
    }
    return true;
}

void FairMQSocketZMQ::Connect(const string& address)
{
    // LOG(INFO) << "connect socket " << fId << " on " << address;

    if (zmq_connect(fSocket, address.c_str()) != 0)
    {
        LOG(ERROR) << "Failed connecting socket " << fId << ", reason: " << zmq_strerror(errno);
        // error here means incorrect configuration. exit if it happens.
        exit(EXIT_FAILURE);
    }
}

int FairMQSocketZMQ::Send(FairMQMessagePtr& msg, const int flags)
{
    int nbytes = -1;

    while (true)
    {
        nbytes = zmq_msg_send(static_cast<zmq_msg_t*>(msg->GetMessage()), fSocket, flags);
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
                continue;
            }
            else
            {
                return -2;
            }
        }
        else if (zmq_errno() == ETERM)
        {
            LOG(INFO) << "terminating socket " << fId;
            return -1;
        }
        else
        {
            LOG(ERROR) << "Failed sending on socket " << fId << ", reason: " << zmq_strerror(errno);
            return nbytes;
        }
    }
}

int FairMQSocketZMQ::Receive(FairMQMessagePtr& msg, const int flags)
{
    int nbytes = -1;
    while (true)
    {
        nbytes = zmq_msg_recv(static_cast<zmq_msg_t*>(msg->GetMessage()), fSocket, flags);
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
                continue;
            }
            else
            {
                return -2;
            }
        }
        else if (zmq_errno() == ETERM)
        {
            LOG(INFO) << "terminating socket " << fId;
            return -1;
        }
        else
        {
            LOG(ERROR) << "Failed receiving on socket " << fId << ", reason: " << zmq_strerror(errno);
            return nbytes;
        }
    }
}

int64_t FairMQSocketZMQ::Send(vector<unique_ptr<FairMQMessage>>& msgVec, const int flags)
{
    const unsigned int vecSize = msgVec.size();

    // Sending vector typicaly handles more then one part
    if (vecSize > 1)
    {
        int64_t totalSize = 0;
        int nbytes = -1;
        bool repeat = false;

        while (true)
        {
            totalSize = 0;
            nbytes = -1;
            repeat = false;

            for (unsigned int i = 0; i < vecSize; ++i)
            {
                nbytes = zmq_msg_send(static_cast<zmq_msg_t*>(msgVec[i]->GetMessage()),
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
                        LOG(INFO) << "terminating socket " << fId;
                        return -1;
                    }
                    LOG(ERROR) << "Failed sending on socket " << fId << ", reason: " << zmq_strerror(errno);
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
        return Send(msgVec.back(), flags);
    }
    else // if the vector is empty, something might be wrong
    {
        LOG(WARN) << "Will not send empty vector";
        return -1;
    }
}

int64_t FairMQSocketZMQ::Receive(vector<unique_ptr<FairMQMessage>>& msgVec, const int flags)
{
    int64_t totalSize = 0;
    int64_t more = 0;
    bool repeat = false;

    while (true)
    {
        // Warn if the vector is filled before Receive() and empty it.
        // if (msgVec.size() > 0)
        // {
        //     LOG(WARN) << "Message vector contains elements before Receive(), they will be deleted!";
        //     msgVec.clear();
        // }

        totalSize = 0;
        more = 0;
        repeat = false;

        do
        {
            unique_ptr<FairMQMessage> part(new FairMQMessageZMQ());

            int nbytes = zmq_msg_recv(static_cast<zmq_msg_t*>(part->GetMessage()), fSocket, flags);
            if (nbytes >= 0)
            {
                msgVec.push_back(move(part));
                totalSize += nbytes;
            }
            else if (zmq_errno() == EAGAIN)
            {
                if (!fInterrupted && ((flags & ZMQ_DONTWAIT) == 0))
                {
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

            size_t more_size = sizeof(more);
            zmq_getsockopt(fSocket, ZMQ_RCVMORE, &more, &more_size);
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
    // LOG(DEBUG) << "Closing socket " << fId;

    if (fSocket == NULL)
    {
        return;
    }

    if (zmq_close(fSocket) != 0)
    {
        LOG(ERROR) << "Failed closing socket " << fId << ", reason: " << zmq_strerror(errno);
    }

    fSocket = NULL;
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

int FairMQSocketZMQ::GetSocket(int) const
{
    // dummy method to comply with the interface. functionality not possible in zeromq.
    return -1;
}

void FairMQSocketZMQ::SetOption(const string& option, const void* value, size_t valueSize)
{
    if (zmq_setsockopt(fSocket, GetConstant(option), value, valueSize) < 0)
    {
        LOG(ERROR) << "Failed setting socket option, reason: " << zmq_strerror(errno);
    }
}

void FairMQSocketZMQ::GetOption(const string& option, void* value, size_t* valueSize)
{
    if (zmq_getsockopt(fSocket, GetConstant(option), value, valueSize) < 0)
    {
        LOG(ERROR) << "Failed getting socket option, reason: " << zmq_strerror(errno);
    }
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

bool FairMQSocketZMQ::SetSendTimeout(const int timeout, const string& address, const string& method)
{
    if (method == "bind")
    {
        if (zmq_unbind(fSocket, address.c_str()) != 0)
        {
            LOG(ERROR) << "Failed unbinding socket " << fId << ", reason: " << zmq_strerror(errno);
            return false;
        }
        if (zmq_setsockopt(fSocket, ZMQ_SNDTIMEO, &timeout, sizeof(int)) != 0)
        {
            LOG(ERROR) << "Failed setting option on socket " << fId << ", reason: " << zmq_strerror(errno);
            return false;
        }
        if (zmq_bind(fSocket, address.c_str()) != 0)
        {
            LOG(ERROR) << "Failed binding socket " << fId << ", reason: " << zmq_strerror(errno);
            return false;
        }
    }
    else if (method == "connect")
    {
        if (zmq_disconnect(fSocket, address.c_str()) != 0)
        {
            LOG(ERROR) << "Failed disconnecting socket " << fId << ", reason: " << zmq_strerror(errno);
            return false;
        }
        if (zmq_setsockopt(fSocket, ZMQ_SNDTIMEO, &timeout, sizeof(int)) != 0)
        {
            LOG(ERROR) << "Failed setting option on socket " << fId << ", reason: " << zmq_strerror(errno);
            return false;
        }
        if (zmq_connect(fSocket, address.c_str()) != 0)
        {
            LOG(ERROR) << "Failed connecting socket " << fId << ", reason: " << zmq_strerror(errno);
            return false;
        }
    }
    else
    {
        LOG(ERROR) << "SetSendTimeout() failed - unknown method provided!";
        return false;
    }

    return true;
}

int FairMQSocketZMQ::GetSendTimeout() const
{
    int timeout = -1;
    size_t size = sizeof(timeout);

    if (zmq_getsockopt(fSocket, ZMQ_SNDTIMEO, &timeout, &size) != 0)
    {
        LOG(ERROR) << "Failed getting option 'receive timeout' on socket " << fId << ", reason: " << zmq_strerror(errno);
    }

    return timeout;
}

bool FairMQSocketZMQ::SetReceiveTimeout(const int timeout, const string& address, const string& method)
{
    if (method == "bind")
    {
        if (zmq_unbind(fSocket, address.c_str()) != 0)
        {
            LOG(ERROR) << "Failed unbinding socket " << fId << ", reason: " << zmq_strerror(errno);
            return false;
        }
        if (zmq_setsockopt(fSocket, ZMQ_RCVTIMEO, &timeout, sizeof(int)) != 0)
        {
            LOG(ERROR) << "Failed setting option on socket " << fId << ", reason: " << zmq_strerror(errno);
            return false;
        }
        if (zmq_bind(fSocket, address.c_str()) != 0)
        {
            LOG(ERROR) << "Failed binding socket " << fId << ", reason: " << zmq_strerror(errno);
            return false;
        }
    }
    else if (method == "connect")
    {
        if (zmq_disconnect(fSocket, address.c_str()) != 0)
        {
            LOG(ERROR) << "Failed disconnecting socket " << fId << ", reason: " << zmq_strerror(errno);
            return false;
        }
        if (zmq_setsockopt(fSocket, ZMQ_RCVTIMEO, &timeout, sizeof(int)) != 0)
        {
            LOG(ERROR) << "Failed setting option on socket " << fId << ", reason: " << zmq_strerror(errno);
            return false;
        }
        if (zmq_connect(fSocket, address.c_str()) != 0)
        {
            LOG(ERROR) << "Failed connecting socket " << fId << ", reason: " << zmq_strerror(errno);
            return false;
        }
    }
    else
    {
        LOG(ERROR) << "SetReceiveTimeout() failed - unknown method provided!";
        return false;
    }

    return true;
}

int FairMQSocketZMQ::GetReceiveTimeout() const
{
    int timeout = -1;
    size_t size = sizeof(timeout);

    if (zmq_getsockopt(fSocket, ZMQ_RCVTIMEO, &timeout, &size) != 0)
    {
        LOG(ERROR) << "Failed getting option 'receive timeout' on socket " << fId << ", reason: " << zmq_strerror(errno);
    }

    return timeout;
}

int FairMQSocketZMQ::GetConstant(const string& constant)
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

    return -1;
}

FairMQSocketZMQ::~FairMQSocketZMQ()
{
}
