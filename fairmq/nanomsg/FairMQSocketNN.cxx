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

#include "FairMQSocketNN.h"
#include "FairMQMessageNN.h"
#include "FairMQLogger.h"
#include "FairMQUnmanagedRegionNN.h"

#include <nanomsg/nn.h>
#include <nanomsg/pipeline.h>
#include <nanomsg/pubsub.h>
#include <nanomsg/reqrep.h>
#include <nanomsg/pair.h>

#include <sstream>
#ifdef MSGPACK_FOUND
#include <msgpack.hpp>
#endif /*MSGPACK_FOUND*/

using namespace std;

atomic<bool> FairMQSocketNN::fInterrupted(false);

FairMQSocketNN::FairMQSocketNN(const string& type, const string& name, const string& id /*= ""*/)
    : FairMQSocket(0, 0, NN_DONTWAIT)
    , fSocket(-1)
    , fId()
    , fBytesTx(0)
    , fBytesRx(0)
    , fMessagesTx(0)
    , fMessagesRx(0)
{
    fId = id + "." + name + "." + type;

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

    int sndTimeout = 700;
    if (nn_setsockopt(fSocket, NN_SOL_SOCKET, NN_SNDTIMEO, &sndTimeout, sizeof(sndTimeout)) != 0)
    {
        LOG(ERROR) << "Failed setting NN_SNDTIMEO socket option, reason: " << nn_strerror(errno);
    }

    int rcvTimeout = 700;
    if (nn_setsockopt(fSocket, NN_SOL_SOCKET, NN_RCVTIMEO, &rcvTimeout, sizeof(rcvTimeout)) != 0)
    {
        LOG(ERROR) << "Failed setting NN_RCVTIMEO socket option, reason: " << nn_strerror(errno);
    }

#ifdef NN_RCVMAXSIZE
    int rcvSize = -1;
    if (nn_setsockopt(fSocket, NN_SOL_SOCKET, NN_RCVMAXSIZE, &rcvSize, sizeof(rcvSize)) != 0)
    {
        LOG(ERROR) << "Failed setting NN_RCVMAXSIZE socket option, reason: " << nn_strerror(errno);
    }
#endif

    // LOG(INFO) << "created socket " << fId;
}

string FairMQSocketNN::GetId()
{
    return fId;
}

bool FairMQSocketNN::Bind(const string& address)
{
    // LOG(INFO) << "bind socket " << fId << " on " << address;

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
    // LOG(INFO) << "connect socket " << fId << " to " << address;

    int eid = nn_connect(fSocket, address.c_str());
    if (eid < 0)
    {
        LOG(ERROR) << "failed connecting socket " << fId << ", reason: " << nn_strerror(errno);
    }
}

int FairMQSocketNN::Send(FairMQMessagePtr& msg, const int flags)
{
    int nbytes = -1;

    while (true)
    {
        void* ptr = msg->GetMessage();
        if (static_cast<FairMQMessageNN*>(msg.get())->fRegionPtr == nullptr)
        {
            nbytes = nn_send(fSocket, &ptr, NN_MSG, flags);
        }
        else
        {
            nbytes = nn_send(fSocket, ptr, msg->GetSize(), flags);
            // nn_send copies the data, safe to call region callback here
            static_cast<FairMQUnmanagedRegionNN*>(static_cast<FairMQMessageNN*>(msg.get())->fRegionPtr)->fCallback(msg->GetMessage(), msg->GetSize());
        }

        if (nbytes >= 0)
        {
            fBytesTx += nbytes;
            ++fMessagesTx;
            static_cast<FairMQMessageNN*>(msg.get())->fReceiving = false;

            return nbytes;
        }
#if NN_VERSION_CURRENT>2 // backwards-compatibility with nanomsg version<=0.6
        else if (nn_errno() == ETIMEDOUT)
#else
        else if (nn_errno() == EAGAIN)
#endif
        {
            if (!fInterrupted && ((flags & NN_DONTWAIT) == 0))
            {
                continue;
            }
            else
            {
                return -2;
            }
        }
        else if (nn_errno() == EAGAIN)
        {
            return -2;
        }
        else if (nn_errno() == ETERM)
        {
            LOG(INFO) << "terminating socket " << fId;
            return -1;
        }
        else
        {
            LOG(ERROR) << "Failed sending on socket " << fId << ", reason: " << nn_strerror(errno);
            return nbytes;
        }
    }
}

int FairMQSocketNN::Receive(FairMQMessagePtr& msg, const int flags)
{
    int nbytes = -1;

    while (true)
    {
        void* ptr = NULL;
        nbytes = nn_recv(fSocket, &ptr, NN_MSG, flags);
        if (nbytes >= 0)
        {
            fBytesRx += nbytes;
            ++fMessagesRx;
            msg->SetMessage(ptr, nbytes);
            static_cast<FairMQMessageNN*>(msg.get())->fReceiving = true;
            return nbytes;
        }
#if NN_VERSION_CURRENT>2 // backwards-compatibility with nanomsg version<=0.6
        else if (nn_errno() == ETIMEDOUT)
#else
        else if (nn_errno() == EAGAIN)
#endif
        {
            if (!fInterrupted && ((flags & NN_DONTWAIT) == 0))
            {
                continue;
            }
            else
            {
                return -2;
            }
        }
        else if (nn_errno() == EAGAIN)
        {
            return -2;
        }
        else if (nn_errno() == ETERM)
        {
            LOG(INFO) << "terminating socket " << fId;
            return -1;
        }
        else
        {
            LOG(ERROR) << "Failed receiving on socket " << fId << ", reason: " << nn_strerror(errno);
            return nbytes;
        }
    }
}

int64_t FairMQSocketNN::Send(vector<unique_ptr<FairMQMessage>>& msgVec, const int flags)
{
    const unsigned int vecSize = msgVec.size();
#ifdef MSGPACK_FOUND

    // create msgpack simple buffer
    msgpack::sbuffer sbuf;
    // create msgpack packer
    msgpack::packer<msgpack::sbuffer> packer(&sbuf);

    // pack all parts into a single msgpack simple buffer
    for (unsigned int i = 0; i < vecSize; ++i)
    {
        static_cast<FairMQMessageNN*>(msgVec[i].get())->fReceiving = false;
        packer.pack_bin(msgVec[i]->GetSize());
        packer.pack_bin_body(static_cast<char*>(msgVec[i]->GetData()), msgVec[i]->GetSize());
        // call region callback
        if (static_cast<FairMQMessageNN*>(msgVec[i].get())->fRegionPtr)
        {
            static_cast<FairMQUnmanagedRegionNN*>(static_cast<FairMQMessageNN*>(msgVec[i].get())->fRegionPtr)->fCallback(msgVec[i]->GetMessage(), msgVec[i]->GetSize());
        }
    }

    int64_t nbytes = -1;

    while (true)
    {
        nbytes = nn_send(fSocket, sbuf.data(), sbuf.size(), flags);
        if (nbytes >= 0)
        {
            fBytesTx += nbytes;
            ++fMessagesTx;
            return nbytes;
        }
#if NN_VERSION_CURRENT>2 // backwards-compatibility with nanomsg version<=0.6
        else if (nn_errno() == ETIMEDOUT)
#else
        else if (nn_errno() == EAGAIN)
#endif
        {
            if (!fInterrupted && ((flags & NN_DONTWAIT) == 0))
            {
                continue;
            }
            else
            {
                return -2;
            }
        }
        else if (nn_errno() == EAGAIN)
        {
            return -2;
        }
        else if (nn_errno() == ETERM)
        {
            LOG(INFO) << "terminating socket " << fId;
            return -1;
        }
        else
        {
            LOG(ERROR) << "Failed sending on socket " << fId << ", reason: " << nn_strerror(errno);
            return nbytes;
        }
    }
#else /*MSGPACK_FOUND*/
    LOG(ERROR) << "Cannot send message from vector of size " << vecSize << " and flags " << flags << " with nanomsg multipart because MessagePack is not available.";
    exit(EXIT_FAILURE);
#endif /*MSGPACK_FOUND*/
}

int64_t FairMQSocketNN::Receive(vector<unique_ptr<FairMQMessage>>& msgVec, const int flags)
{
#ifdef MSGPACK_FOUND
    // Warn if the vector is filled before Receive() and empty it.
    // if (msgVec.size() > 0)
    // {
    //     LOG(WARN) << "Message vector contains elements before Receive(), they will be deleted!";
    //     msgVec.clear();
    // }

    while (true)
    {
        // pointer to point to received message buffer
        char* ptr = NULL;
        // receive the message into a buffer allocated by nanomsg and let ptr point to it
        int nbytes = nn_recv(fSocket, &ptr, NN_MSG, flags);
        if (nbytes >= 0) // if no errors or non-blocking timeouts
        {
            // store statistics on how many bytes received
            fBytesRx += nbytes;
            // store statistics on how many messages received (count messages instead of parts)
            ++fMessagesRx;

            // offset to be used by msgpack to handle separate chunks
            size_t offset = 0;
            while (offset != static_cast<size_t>(nbytes)) // continue until all parts have been read
            {
                // vector of chars to hold blob (unlike char*/void* this type can be converted to by msgpack)
                std::vector<char> buf;

                // unpack and convert chunk
                msgpack::unpacked result;
                unpack(result, ptr, nbytes, offset);
                msgpack::object object(result.get());
                object.convert(buf);
                // get the single message size
                size_t size = buf.size() * sizeof(char);
                unique_ptr<FairMQMessage> part(new FairMQMessageNN(size));
                static_cast<FairMQMessageNN*>(part.get())->fReceiving = true;
                memcpy(part->GetData(), buf.data(), size);
                msgVec.push_back(move(part));
            }

            nn_freemsg(ptr);
            return nbytes;
        }
#if NN_VERSION_CURRENT>2 // backwards-compatibility with nanomsg version<=0.6
        else if (nn_errno() == ETIMEDOUT)
#else
        else if (nn_errno() == EAGAIN)
#endif
        {
            if (!fInterrupted && ((flags & NN_DONTWAIT) == 0))
            {
                continue;
            }
            else
            {
                return -2;
            }
        }
        else if (nn_errno() == EAGAIN)
        {
            return -2;
        }
        else if (nn_errno() == ETERM)
        {
            LOG(INFO) << "terminating socket " << fId;
            return -1;
        }
        else
        {
            LOG(ERROR) << "Failed receiving on socket " << fId << ", reason: " << nn_strerror(errno);
            return nbytes;
        }
    }
#else /*MSGPACK_FOUND*/
    LOG(ERROR) << "Cannot receive message into vector of size " << msgVec.size() << " and flags " << flags << " with nanomsg multipart because MessagePack is not available.";
    exit(EXIT_FAILURE);
#endif /*MSGPACK_FOUND*/
}

void FairMQSocketNN::Close()
{
    nn_close(fSocket);
}

void FairMQSocketNN::Interrupt()
{
    fInterrupted = true;
}

void FairMQSocketNN::Resume()
{
    fInterrupted = false;
}

void* FairMQSocketNN::GetSocket() const
{
    return NULL; // dummy method to comply with the interface. functionality not possible in zeromq.
}

int FairMQSocketNN::GetSocket(int /*nothing*/) const
{
    return fSocket;
}

void FairMQSocketNN::SetOption(const string& option, const void* value, size_t valueSize)
{
    if (option == "snd-size" || option == "rcv-size")
    {
        int val = *(static_cast<int*>(const_cast<void*>(value)));
        if (val <= 0)
        {
            LOG(WARN) << "nanomsg: value for sndKernelSize/rcvKernelSize should be greater than 0, using defaults (128kB).";
            return;
        }
    }

    if (option == "snd-hwm" || option == "rcv-hwm")
    {
        return;
    }

    int rc = nn_setsockopt(fSocket, NN_SOL_SOCKET, GetConstant(option), value, valueSize);
    if (rc < 0)
    {
        LOG(ERROR) << "failed setting socket option, reason: " << nn_strerror(errno);
    }
}

void FairMQSocketNN::GetOption(const string& option, void* value, size_t* valueSize)
{
    int rc = nn_getsockopt(fSocket, NN_SOL_SOCKET, GetConstant(option), value, valueSize);
    if (rc < 0)
    {
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

bool FairMQSocketNN::SetSendTimeout(const int timeout, const string& /*address*/, const string& /*method*/)
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

bool FairMQSocketNN::SetReceiveTimeout(const int timeout, const string& /*address*/, const string& /*method*/)
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
    if (constant == "snd-size")
        return NN_SNDBUF;
    if (constant == "rcv-size")
        return NN_RCVBUF;
    if (constant == "snd-more")
    {
        LOG(ERROR) << "Multipart messages functionality currently not supported by nanomsg!";
        return -1;
    }
    if (constant == "rcv-more")
    {
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
