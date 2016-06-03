/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
#include <sstream>

#include <zmq.h>

#include "FairMQSocketSHM.h"
#include "FairMQMessageSHM.h"
#include "FairMQLogger.h"

using namespace std;
using namespace FairMQ::shmem;

// Context to hold the ZeroMQ sockets
unique_ptr<FairMQContextSHM> FairMQSocketSHM::fContext = unique_ptr<FairMQContextSHM>(new FairMQContextSHM(1));
// bool FairMQSocketSHM::fContextInitialized = false;

FairMQSocketSHM::FairMQSocketSHM(const string& type, const string& name, const int numIoThreads, const string& id /*= ""*/)
    : FairMQSocket(ZMQ_SNDMORE, ZMQ_RCVMORE, ZMQ_DONTWAIT)
    , fSocket(NULL)
    , fId()
    , fBytesTx(0)
    , fBytesRx(0)
    , fMessagesTx(0)
    , fMessagesRx(0)
{
    fId = id + "." + name + "." + type;

    // if (!fContextInitialized)
    // {
    //     fContext = unique_ptr<FairMQContextSHM>(new FairMQContextSHM(1));
    //     fContextInitialized = true;
    // }

    if (zmq_ctx_set(fContext->GetContext(), ZMQ_IO_THREADS, numIoThreads) != 0)
    {
        LOG(ERROR) << "Failed configuring context, reason: " << zmq_strerror(errno);
    }

    fSocket = zmq_socket(fContext->GetContext(), GetConstant(type));

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
    int linger = 500;
    if (zmq_setsockopt(fSocket, ZMQ_LINGER, &linger, sizeof(linger)) != 0)
    {
        LOG(ERROR) << "Failed setting ZMQ_LINGER socket option, reason: " << zmq_strerror(errno);
    }

    int kernelSndSize = 10000;
    if (zmq_setsockopt(fSocket, ZMQ_SNDBUF, &kernelSndSize, sizeof(kernelSndSize)) != 0)
    {
        LOG(ERROR) << "Failed setting ZMQ_SNDBUF socket option, reason: " << zmq_strerror(errno);
    }

    int kernelRcvSize = 10000;
    if (zmq_setsockopt(fSocket, ZMQ_RCVBUF, &kernelRcvSize, sizeof(kernelRcvSize)) != 0)
    {
        LOG(ERROR) << "Failed setting ZMQ_RCVBUF socket option, reason: " << zmq_strerror(errno);
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

string FairMQSocketSHM::GetId()
{
    return fId;
}

bool FairMQSocketSHM::Bind(const string& address)
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

void FairMQSocketSHM::Connect(const string& address)
{
    // LOG(INFO) << "connect socket " << fId << " on " << address;

    if (zmq_connect(fSocket, address.c_str()) != 0)
    {
        LOG(ERROR) << "Failed connecting socket " << fId << ", reason: " << zmq_strerror(errno);
        // error here means incorrect configuration. exit if it happens.
        exit(EXIT_FAILURE);
    }
}

int FairMQSocketSHM::Send(FairMQMessage* msg, const string& flag)
{
    return Send(msg, GetConstant(flag));
}

int FairMQSocketSHM::Send(FairMQMessage* msg, const int flags)
{
    int nbytes = zmq_msg_send(static_cast<zmq_msg_t*>(msg->GetMessage()), fSocket, flags);
    if (nbytes >= 0)
    {
        static_cast<FairMQMessageSHM*>(msg)->fReceiving = false;
        static_cast<FairMQMessageSHM*>(msg)->fQueued = true;
        size_t size = msg->GetSize();

        fBytesTx += size;
        ++fMessagesTx;

        return size;
    }
    if (zmq_errno() == EAGAIN)
    {
        return -2;
    }
    if (zmq_errno() == ETERM)
    {
        LOG(INFO) << "terminating socket " << fId;
        return -1;
    }
    LOG(ERROR) << "Failed sending on socket " << fId << ", reason: " << zmq_strerror(errno);
    return nbytes;
}

int64_t FairMQSocketSHM::Send(const vector<FairMQMessagePtr>& msgVec, const int flags)
{
    // Sending vector typicaly handles more then one part
    if (msgVec.size() > 1)
    {
        int64_t totalSize = 0;

        for (unsigned int i = 0; i < msgVec.size() - 1; ++i)
        {
            int nbytes = zmq_msg_send(static_cast<zmq_msg_t*>(msgVec[i]->GetMessage()), fSocket, ZMQ_SNDMORE|flags);
            if (nbytes >= 0)
            {
                static_cast<FairMQMessageSHM*>(msgVec[i].get())->fReceiving = false;
                static_cast<FairMQMessageSHM*>(msgVec[i].get())->fQueued = true;
                size_t size = msgVec[i]->GetSize();

                totalSize += size;
                fBytesTx += size;
            }
            else
            {
                if (zmq_errno() == EAGAIN)
                {
                    return -2;
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

        int nbytes = zmq_msg_send(static_cast<zmq_msg_t*>(msgVec.back()->GetMessage()), fSocket, flags);
        if (nbytes >= 0)
        {
            static_cast<FairMQMessageSHM*>(msgVec.back().get())->fReceiving = false;
            static_cast<FairMQMessageSHM*>(msgVec.back().get())->fQueued = true;
            size_t size = msgVec.back()->GetSize();

            totalSize += size;
            fBytesTx += size;
        }
        else
        {
            if (zmq_errno() == EAGAIN)
            {
                return -2;
            }
            if (zmq_errno() == ETERM)
            {
                LOG(INFO) << "terminating socket " << fId;
                return -1;
            }
            LOG(ERROR) << "Failed sending on socket " << fId << ", reason: " << zmq_strerror(errno);
            return nbytes;
        }

        // store statistics on how many messages have been sent (handle all parts as a single message)
        ++fMessagesTx;
        return totalSize;
    } // If there's only one part, send it as a regular message
    else if (msgVec.size() == 1)
    {
        return Send(msgVec.back().get(), flags);
    }
    else // if the vector is empty, something might be wrong
    {
        LOG(WARN) << "Will not send empty vector";
        return -1;
    }
}

int FairMQSocketSHM::Receive(FairMQMessage* msg, const string& flag)
{
    return Receive(msg, GetConstant(flag));
}

int FairMQSocketSHM::Receive(FairMQMessage* msg, const int flags)
{
    zmq_msg_t* msgPtr = static_cast<zmq_msg_t*>(msg->GetMessage());
    int nbytes = zmq_msg_recv(msgPtr, fSocket, flags);
    if (nbytes == 0)
    {
        ++fMessagesRx;
        return nbytes;
    }
    else if (nbytes > 0)
    {
        string ownerID(static_cast<char*>(zmq_msg_data(msgPtr)), zmq_msg_size(msgPtr));
        ShPtrOwner* owner = Manager::Instance().Segment()->find<ShPtrOwner>(ownerID.c_str()).first;
        size_t size = 0;
        if (owner)
        {
            static_cast<FairMQMessageSHM*>(msg)->fOwner = owner;
            static_cast<FairMQMessageSHM*>(msg)->fReceiving = true;
            size = msg->GetSize();

            fBytesRx += size;
            ++fMessagesRx;

            return size;
        }
        else
        {
            LOG(ERROR) << "Received meta data, but could not find corresponding chunk";
            return -1;
        }
    }
    if (zmq_errno() == EAGAIN)
    {
        return -2;
    }
    if (zmq_errno() == ETERM)
    {
        LOG(INFO) << "terminating socket " << fId;
        return -1;
    }
    LOG(ERROR) << "Failed receiving on socket " << fId << ", reason: " << zmq_strerror(errno);
    return nbytes;
}

int64_t FairMQSocketSHM::Receive(vector<FairMQMessagePtr>& msgVec, const int flags)
{
    // Warn if the vector is filled before Receive() and empty it.
    if (msgVec.size() > 0)
    {
        LOG(WARN) << "Message vector contains elements before Receive(), they will be deleted!";
        msgVec.clear();
    }

    int64_t totalSize = 0;
    int64_t more = 0;

    do
    {
        FairMQMessagePtr part(new FairMQMessageSHM());
        zmq_msg_t* msgPtr = static_cast<zmq_msg_t*>(part->GetMessage());

        int nbytes = zmq_msg_recv(msgPtr, fSocket, flags);
        if (nbytes == 0)
        {
            msgVec.push_back(move(part));
        }
        else if (nbytes > 0)
        {
            string ownerID(static_cast<char*>(zmq_msg_data(msgPtr)), zmq_msg_size(msgPtr));
            ShPtrOwner* owner = Manager::Instance().Segment()->find<ShPtrOwner>(ownerID.c_str()).first;
            size_t size = 0;
            if (owner)
            {
                static_cast<FairMQMessageSHM*>(part.get())->fOwner = owner;
                static_cast<FairMQMessageSHM*>(part.get())->fReceiving = true;
                size = part->GetSize();

                msgVec.push_back(move(part));

                fBytesRx += size;
                totalSize += size;
            }
            else
            {
                LOG(ERROR) << "Received meta data, but could not find corresponding chunk";
                return -1;
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

    // store statistics on how many messages have been received (handle all parts as a single message)
    ++fMessagesRx;
    return totalSize;
}

void FairMQSocketSHM::Close()
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

void FairMQSocketSHM::Terminate()
{
    if (zmq_ctx_destroy(fContext->GetContext()) != 0)
    {
        LOG(ERROR) << "Failed terminating context, reason: " << zmq_strerror(errno);
    }
}

void FairMQSocketSHM::Interrupt()
{
    FairMQMessageSHM::fInterrupted = true;
}

void FairMQSocketSHM::Resume()
{
    FairMQMessageSHM::fInterrupted = false;
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
        LOG(ERROR) << "Failed setting socket option, reason: " << zmq_strerror(errno);
    }
}

void FairMQSocketSHM::GetOption(const string& option, void* value, size_t* valueSize)
{
    if (zmq_getsockopt(fSocket, GetConstant(option), value, valueSize) < 0)
    {
        LOG(ERROR) << "Failed getting socket option, reason: " << zmq_strerror(errno);
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

int FairMQSocketSHM::GetSendTimeout() const
{
    int timeout = -1;
    size_t size = sizeof(timeout);

    if (zmq_getsockopt(fSocket, ZMQ_SNDTIMEO, &timeout, &size) != 0)
    {
        LOG(ERROR) << "Failed getting option 'receive timeout' on socket " << fId << ", reason: " << zmq_strerror(errno);
    }

    return timeout;
}

bool FairMQSocketSHM::SetReceiveTimeout(const int timeout, const string& address, const string& method)
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

int FairMQSocketSHM::GetReceiveTimeout() const
{
    int timeout = -1;
    size_t size = sizeof(timeout);

    if (zmq_getsockopt(fSocket, ZMQ_RCVTIMEO, &timeout, &size) != 0)
    {
        LOG(ERROR) << "Failed getting option 'receive timeout' on socket " << fId << ", reason: " << zmq_strerror(errno);
    }

    return timeout;
}

int FairMQSocketSHM::GetConstant(const string& constant)
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

FairMQSocketSHM::~FairMQSocketSHM()
{
}
