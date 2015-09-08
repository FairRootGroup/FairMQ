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

#include "FairMQSocketZMQ.h"
#include "FairMQLogger.h"

using namespace std;

// Context to hold the ZeroMQ sockets
boost::shared_ptr<FairMQContextZMQ> FairMQSocketZMQ::fContext = boost::shared_ptr<FairMQContextZMQ>(new FairMQContextZMQ(1));

FairMQSocketZMQ::FairMQSocketZMQ(const string& type, const string& name, int numIoThreads)
    : FairMQSocket(ZMQ_SNDMORE, ZMQ_RCVMORE, ZMQ_DONTWAIT)
    , fSocket(NULL)
    , fId()
    , fBytesTx(0)
    , fBytesRx(0)
    , fMessagesTx(0)
    , fMessagesRx(0)
{
    fId = name + "." + type;

    if (zmq_ctx_set(fContext->GetContext(), ZMQ_IO_THREADS, numIoThreads) != 0)
    {
        LOG(ERROR) << "failed configuring context, reason: " << zmq_strerror(errno);
    }

    fSocket = zmq_socket(fContext->GetContext(), GetConstant(type));

    if (fSocket == NULL)
    {
        LOG(ERROR) << "failed creating socket " << fId << ", reason: " << zmq_strerror(errno);
        exit(EXIT_FAILURE);
    }

    if (zmq_setsockopt(fSocket, ZMQ_IDENTITY, &fId, fId.length()) != 0)
    {
        LOG(ERROR) << "failed setting ZMQ_IDENTITY socket option, reason: " << zmq_strerror(errno);
    }

    // Tell socket to try and send/receive outstanding messages for <linger> milliseconds before terminating.
    // Default value for ZeroMQ is -1, which is to wait forever.
    int linger = 500;
    if (zmq_setsockopt(fSocket, ZMQ_LINGER, &linger, sizeof(linger)) != 0)
    {
        LOG(ERROR) << "failed setting ZMQ_LINGER socket option, reason: " << zmq_strerror(errno);
    }

    if (type == "sub")
    {
        if (zmq_setsockopt(fSocket, ZMQ_SUBSCRIBE, NULL, 0) != 0)
        {
            LOG(ERROR) << "failed setting ZMQ_SUBSCRIBE socket option, reason: " << zmq_strerror(errno);
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
        LOG(ERROR) << "failed binding socket " << fId << ", reason: " << zmq_strerror(errno);
        return false;
    }
    return true;
}

void FairMQSocketZMQ::Connect(const string& address)
{
    // LOG(INFO) << "connect socket " << fId << " on " << address;

    if (zmq_connect(fSocket, address.c_str()) != 0)
    {
        LOG(ERROR) << "failed connecting socket " << fId << ", reason: " << zmq_strerror(errno);
        // error here means incorrect configuration. exit if it happens.
        exit(EXIT_FAILURE);
    }
}

int FairMQSocketZMQ::Send(FairMQMessage* msg, const string& flag)
{
    int nbytes = zmq_msg_send(static_cast<zmq_msg_t*>(msg->GetMessage()), fSocket, GetConstant(flag));
    if (nbytes >= 0)
    {
        fBytesTx += nbytes;
        ++fMessagesTx;
        return nbytes;
    }
    if (zmq_errno() == EAGAIN)
    {
        return 0;
    }
    if (zmq_errno() == ETERM)
    {
        LOG(INFO) << "terminating socket " << fId;
        return -1;
    }
    LOG(ERROR) << "failed sending on socket " << fId << ", reason: " << zmq_strerror(errno);
    return nbytes;
}

int FairMQSocketZMQ::Send(FairMQMessage* msg, const int flags)
{
    int nbytes = zmq_msg_send(static_cast<zmq_msg_t*>(msg->GetMessage()), fSocket, flags);
    if (nbytes >= 0)
    {
        fBytesTx += nbytes;
        ++fMessagesTx;
        return nbytes;
    }
    if (zmq_errno() == EAGAIN)
    {
        return 0;
    }
    if (zmq_errno() == ETERM)
    {
        LOG(INFO) << "terminating socket " << fId;
        return -1;
    }
    LOG(ERROR) << "failed sending on socket " << fId << ", reason: " << zmq_strerror(errno);
    return nbytes;
}

int FairMQSocketZMQ::Receive(FairMQMessage* msg, const string& flag)
{
    int nbytes = zmq_msg_recv(static_cast<zmq_msg_t*>(msg->GetMessage()), fSocket, GetConstant(flag));
    if (nbytes >= 0)
    {
        fBytesRx += nbytes;
        ++fMessagesRx;
        return nbytes;
    }
    if (zmq_errno() == EAGAIN)
    {
        return 0;
    }
    if (zmq_errno() == ETERM)
    {
        LOG(INFO) << "terminating socket " << fId;
        return -1;
    }
    LOG(ERROR) << "failed receiving on socket " << fId << ", reason: " << zmq_strerror(errno);
    return nbytes;
}

int FairMQSocketZMQ::Receive(FairMQMessage* msg, const int flags)
{
    int nbytes = zmq_msg_recv(static_cast<zmq_msg_t*>(msg->GetMessage()), fSocket, flags);
    if (nbytes >= 0)
    {
        fBytesRx += nbytes;
        ++fMessagesRx;
        return nbytes;
    }
    if (zmq_errno() == EAGAIN)
    {
        return 0;
    }
    if (zmq_errno() == ETERM)
    {
        LOG(INFO) << "terminating socket " << fId;
        return -1;
    }
    LOG(ERROR) << "failed receiving on socket " << fId << ", reason: " << zmq_strerror(errno);
    return nbytes;
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
        LOG(ERROR) << "failed closing socket " << fId << ", reason: " << zmq_strerror(errno);
    }

    fSocket = NULL;
}

void FairMQSocketZMQ::Terminate()
{
    if (zmq_ctx_destroy(fContext->GetContext()) != 0)
    {
        LOG(ERROR) << "failed terminating context, reason: " << zmq_strerror(errno);
    }
}

void* FairMQSocketZMQ::GetSocket()
{
    return fSocket;
}

int FairMQSocketZMQ::GetSocket(int nothing)
{
    // dummy method to comply with the interface. functionality not possible in zeromq.
    return -1;
}

void FairMQSocketZMQ::SetOption(const string& option, const void* value, size_t valueSize)
{
    if (zmq_setsockopt(fSocket, GetConstant(option), value, valueSize) < 0)
    {
        LOG(ERROR) << "failed setting socket option, reason: " << zmq_strerror(errno);
    }
}

void FairMQSocketZMQ::GetOption(const string& option, void* value, size_t* valueSize)
{
    if (zmq_getsockopt(fSocket, GetConstant(option), value, valueSize) < 0)
    {
        LOG(ERROR) << "failed getting socket option, reason: " << zmq_strerror(errno);
    }
}

unsigned long FairMQSocketZMQ::GetBytesTx()
{
    return fBytesTx;
}

unsigned long FairMQSocketZMQ::GetBytesRx()
{
    return fBytesRx;
}

unsigned long FairMQSocketZMQ::GetMessagesTx()
{
    return fMessagesTx;
}

unsigned long FairMQSocketZMQ::GetMessagesRx()
{
    return fMessagesRx;
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

FairMQSocketZMQ::~FairMQSocketZMQ()
{
}
