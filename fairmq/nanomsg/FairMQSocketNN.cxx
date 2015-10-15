/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQSocketNN.cxx
 *
 * @since 2012-12-05
 * @author A. Rybalchenko
 */

#include <sstream>

#include "FairMQSocketNN.h"
#include "FairMQMessageNN.h"
#include "FairMQLogger.h"

using namespace std;

FairMQSocketNN::FairMQSocketNN(const string& type, const std::string& name, int numIoThreads)
    : FairMQSocket(0, 0, NN_DONTWAIT)
    , fSocket(-1)
    , fId()
    , fBytesTx(0)
    , fBytesRx(0)
    , fMessagesTx(0)
    , fMessagesRx(0)
{
    fId = name + "." + type;

    if (numIoThreads > 1)
    {
        LOG(INFO) << "number of I/O threads is not used in nanomsg";
    }

    if (type == "router" || type == "dealer")
    {
        // Additional info about using the sockets ROUTER and DEALER with nanomsg can be found in:
        // http://250bpm.com/blog:14
        // http://www.freelists.org/post/nanomsg/a-stupid-load-balancing-question,1
        fSocket = nn_socket(AF_SP_RAW, GetConstant(type));
        if (fSocket == -1)
        {
            LOG(ERROR) << "failed creating socket " << fId << ", reason: " << nn_strerror(errno);
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        fSocket = nn_socket(AF_SP, GetConstant(type));
        if (fSocket == -1)
        {
            LOG(ERROR) << "failed creating socket " << fId << ", reason: " << nn_strerror(errno);
            exit(EXIT_FAILURE);
        }
        if (type == "sub")
        {
            nn_setsockopt(fSocket, NN_SUB, NN_SUB_SUBSCRIBE, NULL, 0);
        }
    }

    LOG(INFO) << "created socket " << fId;
}

string FairMQSocketNN::GetId()
{
    return fId;
}

bool FairMQSocketNN::Bind(const string& address)
{
    LOG(INFO) << "bind socket " << fId << " on " << address;

    int eid = nn_bind(fSocket, address.c_str());
    if (eid < 0)
    {
        LOG(ERROR) << "failed binding socket " << fId << ", reason: " << nn_strerror(errno);
        return false;
    }
    return true;
}

void FairMQSocketNN::Connect(const string& address)
{
    LOG(INFO) << "connect socket " << fId << " to " << address;

    int eid = nn_connect(fSocket, address.c_str());
    if (eid < 0)
    {
        LOG(ERROR) << "failed connecting socket " << fId << ", reason: " << nn_strerror(errno);
    }
}

int FairMQSocketNN::Send(FairMQMessage* msg, const string& flag)
{
    void* ptr = msg->GetMessage();
    int nbytes = nn_send(fSocket, &ptr, NN_MSG, GetConstant(flag));
    if (nbytes >= 0)
    {
        fBytesTx += nbytes;
        ++fMessagesTx;
        static_cast<FairMQMessageNN*>(msg)->fReceiving = false;
    }
    if (nn_errno() == EAGAIN)
    {
        return -2;
    }
    if (nn_errno() == ETERM)
    {
        LOG(INFO) << "terminating socket " << fId;
        return -1;
    }
    LOG(ERROR) << "Failed sending on socket " << fId << ", reason: " << nn_strerror(errno);
    return nbytes;
}

int FairMQSocketNN::Send(FairMQMessage* msg, const int flags)
{
    void* ptr = msg->GetMessage();
    int nbytes = nn_send(fSocket, &ptr, NN_MSG, flags);
    if (nbytes >= 0)
    {
        fBytesTx += nbytes;
        ++fMessagesTx;
        static_cast<FairMQMessageNN*>(msg)->fReceiving = false;
    }
    if (nn_errno() == EAGAIN)
    {
        return -2;
    }
    if (nn_errno() == ETERM)
    {
        LOG(INFO) << "terminating socket " << fId;
        return -1;
    }
    LOG(ERROR) << "Failed sending on socket " << fId << ", reason: " << nn_strerror(errno);
    return nbytes;
}

int FairMQSocketNN::Receive(FairMQMessage* msg, const string& flag)
{
    void* ptr = NULL;
    int nbytes = nn_recv(fSocket, &ptr, NN_MSG, GetConstant(flag));
    if (nbytes >= 0)
    {
        fBytesRx += nbytes;
        ++fMessagesRx;
        msg->Rebuild();
        msg->SetMessage(ptr, nbytes);
        static_cast<FairMQMessageNN*>(msg)->fReceiving = true;
    }
    if (nn_errno() == EAGAIN)
    {
        return -2;
    }
    if (nn_errno() == ETERM)
    {
        LOG(INFO) << "terminating socket " << fId;
        return -1;
    }
    LOG(ERROR) << "Failed receiving on socket " << fId << ", reason: " << nn_strerror(errno);
    return nbytes;
}

int FairMQSocketNN::Receive(FairMQMessage* msg, const int flags)
{
    void* ptr = NULL;
    int nbytes = nn_recv(fSocket, &ptr, NN_MSG, flags);
    if (nbytes >= 0)
    {
        fBytesRx += nbytes;
        ++fMessagesRx;
        msg->SetMessage(ptr, nbytes);
        static_cast<FairMQMessageNN*>(msg)->fReceiving = true;
    }
    if (nn_errno() == EAGAIN)
    {
        return -2;
    }
    if (nn_errno() == ETERM)
    {
        LOG(INFO) << "terminating socket " << fId;
        return -1;
    }
    LOG(ERROR) << "Failed receiving on socket " << fId << ", reason: " << nn_strerror(errno);
    return nbytes;
}

void FairMQSocketNN::Close()
{
    nn_close(fSocket);
}

void FairMQSocketNN::Terminate()
{
    nn_term();
}

void* FairMQSocketNN::GetSocket() const
{
    return NULL; // dummy method to comply with the interface. functionality not possible in zeromq.
}

int FairMQSocketNN::GetSocket(int nothing) const
{
    return fSocket;
}

void FairMQSocketNN::SetOption(const string& option, const void* value, size_t valueSize)
{
    int rc = nn_setsockopt(fSocket, NN_SOL_SOCKET, GetConstant(option), value, valueSize);
    if (rc < 0)
    {
        LOG(ERROR) << "failed setting socket option, reason: " << nn_strerror(errno);
    }
}

void FairMQSocketNN::GetOption(const string& option, void* value, size_t* valueSize)
{
    int rc = nn_getsockopt(fSocket, NN_SOL_SOCKET, GetConstant(option), value, valueSize);
    if (rc < 0) {
        LOG(ERROR) << "failed getting socket option, reason: " << nn_strerror(errno);
    }
}

unsigned long FairMQSocketNN::GetBytesTx() const
{
    return fBytesTx;
}

unsigned long FairMQSocketNN::GetBytesRx() const
{
    return fBytesRx;
}

unsigned long FairMQSocketNN::GetMessagesTx() const
{
    return fMessagesTx;
}

unsigned long FairMQSocketNN::GetMessagesRx() const
{
    return fMessagesRx;
}

bool FairMQSocketNN::SetSendTimeout(const int timeout, const string& address, const string& method)
{
    if (nn_setsockopt(fSocket, NN_SOL_SOCKET, NN_SNDTIMEO, &timeout, sizeof(int)) != 0)
    {
        LOG(ERROR) << "Failed setting option 'send timeout' on socket " << fId << ", reason: " << nn_strerror(errno);
        return false;
    }

    return true;
}

int FairMQSocketNN::GetSendTimeout() const
{
    int timeout = -1;
    size_t size = sizeof(timeout);

    if (nn_getsockopt(fSocket, NN_SOL_SOCKET, NN_SNDTIMEO, &timeout, &size) != 0)
    {
        LOG(ERROR) << "Failed getting option 'send timeout' on socket " << fId << ", reason: " << nn_strerror(errno);
    }

    return timeout;
}

bool FairMQSocketNN::SetReceiveTimeout(const int timeout, const string& address, const string& method)
{
    if (nn_setsockopt(fSocket, NN_SOL_SOCKET, NN_RCVTIMEO, &timeout, sizeof(int)) != 0)
    {
        LOG(ERROR) << "Failed setting option 'receive timeout' on socket " << fId << ", reason: " << nn_strerror(errno);
        return false;
    }

    return true;
}

int FairMQSocketNN::GetReceiveTimeout() const
{
    int timeout = -1;
    size_t size = sizeof(timeout);

    if (nn_getsockopt(fSocket, NN_SOL_SOCKET, NN_RCVTIMEO, &timeout, &size) != 0)
    {
        LOG(ERROR) << "Failed getting option 'receive timeout' on socket " << fId << ", reason: " << nn_strerror(errno);
    }

    return timeout;
}

int FairMQSocketNN::GetConstant(const string& constant)
{
    if (constant == "")
        return 0;
    if (constant == "sub")
        return NN_SUB;
    if (constant == "pub")
        return NN_PUB;
    if (constant == "xsub")
        return NN_SUB;
    if (constant == "xpub")
        return NN_PUB;
    if (constant == "push")
        return NN_PUSH;
    if (constant == "pull")
        return NN_PULL;
    if (constant == "req")
        return NN_REQ;
    if (constant == "rep")
        return NN_REP;
    if (constant == "dealer")
        return NN_REQ;
    if (constant == "router")
        return NN_REP;
    if (constant == "pair")
        return NN_PAIR;

    if (constant == "snd-hwm")
        return NN_SNDBUF;
    if (constant == "rcv-hwm")
        return NN_RCVBUF;
    if (constant == "snd-more") {
        LOG(ERROR) << "Multipart messages functionality currently not supported by nanomsg!";
        return -1;
    }
    if (constant == "rcv-more") {
        LOG(ERROR) << "Multipart messages functionality currently not supported by nanomsg!";
        return -1;
    }

    if (constant == "linger")
        return NN_LINGER;
    if (constant == "no-block")
        return NN_DONTWAIT;

    return -1;
}

FairMQSocketNN::~FairMQSocketNN()
{
    Close();
}
