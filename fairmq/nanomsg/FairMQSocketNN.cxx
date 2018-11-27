/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *              GNU Lesser General Public Licence (LGPL) version 3,             *  
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
#include <fairmq/Tools.h>

#include <nanomsg/nn.h>
#include <nanomsg/pipeline.h>
#include <nanomsg/pubsub.h>
#include <nanomsg/reqrep.h>
#include <nanomsg/pair.h>

#include <sstream>
#include <msgpack.hpp>

using namespace std;
using namespace fair::mq;

atomic<bool> FairMQSocketNN::fInterrupted(false);

FairMQSocketNN::FairMQSocketNN(const string& type, const string& name, const string& id /*= ""*/, FairMQTransportFactory* fac /*=nullptr*/)
    : FairMQSocket{fac}
    , fSocket(-1)
    , fId(id + "." + name + "." + type)
    , fBytesTx(0)
    , fBytesRx(0)
    , fMessagesTx(0)
    , fMessagesRx(0)
    , fSndTimeout(100)
    , fRcvTimeout(100)
    , fLinger(500)
{
    if (type == "router" || type == "dealer")
    {
        // Additional info about using the sockets ROUTER and DEALER with nanomsg can be found in:
        // http://250bpm.com/blog:14
        // http://www.freelists.org/post/nanomsg/a-stupid-load-balancing-question,1
        fSocket = nn_socket(AF_SP_RAW, GetConstant(type));
        if (fSocket == -1)
        {
            LOG(error) << "failed creating socket " << fId << ", reason: " << nn_strerror(errno);
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        fSocket = nn_socket(AF_SP, GetConstant(type));
        if (fSocket == -1)
        {
            LOG(error) << "failed creating socket " << fId << ", reason: " << nn_strerror(errno);
            exit(EXIT_FAILURE);
        }
        if (type == "sub")
        {
            nn_setsockopt(fSocket, NN_SUB, NN_SUB_SUBSCRIBE, nullptr, 0);
        }
    }

    if (nn_setsockopt(fSocket, NN_SOL_SOCKET, NN_SNDTIMEO, &fSndTimeout, sizeof(fSndTimeout)) != 0)
    {
        LOG(error) << "Failed setting NN_SNDTIMEO socket option, reason: " << nn_strerror(errno);
    }

    if (nn_setsockopt(fSocket, NN_SOL_SOCKET, NN_RCVTIMEO, &fRcvTimeout, sizeof(fRcvTimeout)) != 0)
    {
        LOG(error) << "Failed setting NN_RCVTIMEO socket option, reason: " << nn_strerror(errno);
    }

#ifdef NN_RCVMAXSIZE
    int rcvSize = -1;
    if (nn_setsockopt(fSocket, NN_SOL_SOCKET, NN_RCVMAXSIZE, &rcvSize, sizeof(rcvSize)) != 0)
    {
        LOG(error) << "Failed setting NN_RCVMAXSIZE socket option, reason: " << nn_strerror(errno);
    }
#endif

    // LOG(info) << "created socket " << fId;
}

string FairMQSocketNN::GetId()
{
    return fId;
}

bool FairMQSocketNN::Bind(const string& address)
{
    // LOG(info) << "bind socket " << fId << " on " << address;

    if (nn_bind(fSocket, address.c_str()) < 0)
    {
        LOG(error) << "failed binding socket " << fId << ", reason: " << nn_strerror(errno);
        return false;
    }

    return true;
}

bool FairMQSocketNN::Connect(const string& address)
{
    // LOG(info) << "connect socket " << fId << " to " << address;

    if (nn_connect(fSocket, address.c_str()) < 0)
    {
        LOG(error) << "failed connecting socket " << fId << ", reason: " << nn_strerror(errno);
        return false;
    }

    return true;
}

int FairMQSocketNN::Send(FairMQMessagePtr& msg, const int timeout)
{
    int flags = 0;
    if (timeout == 0)
    {
        flags = NN_DONTWAIT;
    }
    int nbytes = -1;
    int elapsed = 0;

    FairMQMessageNN* msgPtr = static_cast<FairMQMessageNN*>(msg.get());
    void* bufPtr = msgPtr->GetMessage();

    while (true)
    {
        if (msgPtr->fRegionPtr == nullptr)
        {
            nbytes = nn_send(fSocket, &bufPtr, NN_MSG, flags);
        }
        else
        {
            nbytes = nn_send(fSocket, bufPtr, msg->GetSize(), flags);
            // nn_send copies the data, safe to call region callback here
            static_cast<FairMQUnmanagedRegionNN*>(msgPtr->fRegionPtr)->fCallback(bufPtr, msg->GetSize(), reinterpret_cast<void*>(msgPtr->fHint));
        }

        if (nbytes >= 0)
        {
            fBytesTx += nbytes;
            ++fMessagesTx;
            static_cast<FairMQMessageNN*>(msg.get())->fReceiving = false;

            return nbytes;
        }
        else if (nn_errno() == ETIMEDOUT)
        {
            if (!fInterrupted && ((flags & NN_DONTWAIT) == 0))
            {
                if (timeout > 0)
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
        else if (nn_errno() == EAGAIN)
        {
            return -2;
        }
        else if (nn_errno() == ETERM)
        {
            LOG(info) << "terminating socket " << fId;
            return -1;
        }
        else
        {
            LOG(error) << "Failed sending on socket " << fId << ", reason: " << nn_strerror(errno);
            return nbytes;
        }
    }
}

int FairMQSocketNN::Receive(FairMQMessagePtr& msg, const int timeout)
{
    int flags = 0;
    if (timeout == 0)
    {
        flags = NN_DONTWAIT;
    }
    int elapsed = 0;

    FairMQMessageNN* msgPtr = static_cast<FairMQMessageNN*>(msg.get());

    while (true)
    {
        void* ptr = nullptr;
        int nbytes = nn_recv(fSocket, &ptr, NN_MSG, flags);
        if (nbytes >= 0)
        {
            fBytesRx += nbytes;
            ++fMessagesRx;
            msgPtr->SetMessage(ptr, nbytes);
            msgPtr->fReceiving = true;
            return nbytes;
        }
        else if (nn_errno() == ETIMEDOUT)
        {
            if (!fInterrupted && ((flags & NN_DONTWAIT) == 0))
            {
                if (timeout > 0)
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
        else if (nn_errno() == EAGAIN)
        {
            return -2;
        }
        else if (nn_errno() == ETERM)
        {
            LOG(info) << "terminating socket " << fId;
            return -1;
        }
        else
        {
            LOG(error) << "Failed receiving on socket " << fId << ", reason: " << nn_strerror(errno);
            return nbytes;
        }
    }
}

int64_t FairMQSocketNN::Send(vector<FairMQMessagePtr>& msgVec, const int timeout)
{
    int flags = 0;
    if (timeout == 0)
    {
        flags = NN_DONTWAIT;
    }
    const unsigned int vecSize = msgVec.size();
    int elapsed = 0;

    // create msgpack simple buffer
    msgpack::sbuffer sbuf;
    // create msgpack packer
    msgpack::packer<msgpack::sbuffer> packer(&sbuf);

    // pack all parts into a single msgpack simple buffer
    for (unsigned int i = 0; i < vecSize; ++i)
    {
        FairMQMessageNN* partPtr = static_cast<FairMQMessageNN*>(msgVec[i].get());

        partPtr->fReceiving = false;
        packer.pack_bin(msgVec[i]->GetSize());
        packer.pack_bin_body(static_cast<char*>(msgVec[i]->GetData()), msgVec[i]->GetSize());
        // call region callback
        if (partPtr->fRegionPtr)
        {
            static_cast<FairMQUnmanagedRegionNN*>(partPtr->fRegionPtr)->fCallback(partPtr->GetMessage(), partPtr->GetSize(), reinterpret_cast<void*>(partPtr->fHint));
        }
    }

    while (true)
    {
        int64_t nbytes = nn_send(fSocket, sbuf.data(), sbuf.size(), flags);
        if (nbytes >= 0)
        {
            fBytesTx += nbytes;
            ++fMessagesTx;
            return nbytes;
        }
        else if (nn_errno() == ETIMEDOUT)
        {
            if (!fInterrupted && ((flags & NN_DONTWAIT) == 0))
            {
                if (timeout > 0)
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
        else if (nn_errno() == EAGAIN)
        {
            return -2;
        }
        else if (nn_errno() == ETERM)
        {
            LOG(info) << "terminating socket " << fId;
            return -1;
        }
        else
        {
            LOG(error) << "Failed sending on socket " << fId << ", reason: " << nn_strerror(errno);
            return nbytes;
        }
    }
}

int64_t FairMQSocketNN::Receive(vector<FairMQMessagePtr>& msgVec, const int timeout)
{
    int flags = 0;
    if (timeout == 0)
    {
        flags = NN_DONTWAIT;
    }
    // Warn if the vector is filled before Receive() and empty it.
    // if (msgVec.size() > 0)
    // {
    //     LOG(warn) << "Message vector contains elements before Receive(), they will be deleted!";
    //     msgVec.clear();
    // }

    int elapsed = 0;

    while (true)
    {
        // pointer to point to received message buffer
        char* ptr = nullptr;
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
                vector<char> buf;

                // unpack and convert chunk
                msgpack::unpacked result;
                unpack(result, ptr, nbytes, offset);
                msgpack::object object(result.get());
                object.convert(buf);
                // get the single message size
                size_t size = buf.size() * sizeof(char);
                FairMQMessagePtr part(new FairMQMessageNN(size, GetTransport()));
                static_cast<FairMQMessageNN*>(part.get())->fReceiving = true;
                memcpy(part->GetData(), buf.data(), size);
                msgVec.push_back(move(part));
            }

            nn_freemsg(ptr);
            return nbytes;
        }
        else if (nn_errno() == ETIMEDOUT)
        {
            if (!fInterrupted && ((flags & NN_DONTWAIT) == 0))
            {
                if (timeout > 0)
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
        else if (nn_errno() == EAGAIN)
        {
            return -2;
        }
        else if (nn_errno() == ETERM)
        {
            LOG(info) << "terminating socket " << fId;
            return -1;
        }
        else
        {
            LOG(error) << "Failed receiving on socket " << fId << ", reason: " << nn_strerror(errno);
            return nbytes;
        }
    }
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

int FairMQSocketNN::GetSocket() const
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
            LOG(warn) << "value for sndKernelSize/rcvKernelSize should be greater than 0, leaving unchanged.";
            return;
        }
    }

    if (option == "snd-hwm" || option == "rcv-hwm")
    {
        return;
    }

    if (option == "linger")
    {
        fLinger = *static_cast<int*>(const_cast<void*>(value));
        return;
    }

    int rc = nn_setsockopt(fSocket, NN_SOL_SOCKET, GetConstant(option), value, valueSize);
    if (rc < 0)
    {
        LOG(error) << "failed setting socket option, reason: " << nn_strerror(errno);
    }
}

void FairMQSocketNN::GetOption(const string& option, void* value, size_t* valueSize)
{
    if (option == "linger")
    {
        *static_cast<int*>(value) = fLinger;
        return;
    }

    if (option == "snd-hwm" || option == "rcv-hwm")
    {
        *static_cast<int*>(value) = -1;
        return;
    }


    int rc = nn_getsockopt(fSocket, NN_SOL_SOCKET, GetConstant(option), value, valueSize);
    if (rc < 0)
    {
        LOG(error) << "failed getting socket option, reason: " << nn_strerror(errno);
    }
}

void FairMQSocketNN::SetLinger(const int value)
{
    fLinger = value;
}

int FairMQSocketNN::GetLinger() const
{
    return fLinger;
}

void FairMQSocketNN::SetSndBufSize(const int /* value */)
{
    // not used in nanomsg
}

int FairMQSocketNN::GetSndBufSize() const
{
    // not used in nanomsg
    return -1;
}

void FairMQSocketNN::SetRcvBufSize(const int /* value */)
{
    // not used in nanomsg
}

int FairMQSocketNN::GetRcvBufSize() const
{
    // not used in nanomsg
    return -1;
}

void FairMQSocketNN::SetSndKernelSize(const int value)
{
    if (nn_setsockopt(fSocket, NN_SOL_SOCKET, NN_SNDBUF, &value, sizeof(value)) < 0) {
        throw SocketError(tools::ToString("failed setting NN_SNDBUF, reason: ", nn_strerror(errno)));
    }
}

int FairMQSocketNN::GetSndKernelSize() const
{
    int value = 0;
    size_t valueSize;
    if (nn_getsockopt(fSocket, NN_SOL_SOCKET, NN_SNDBUF, &value, &valueSize) < 0) {
        throw SocketError(tools::ToString("failed getting NN_SNDBUF, reason: ", nn_strerror(errno)));
    }
    return value;
}

void FairMQSocketNN::SetRcvKernelSize(const int value)
{
    if (nn_setsockopt(fSocket, NN_SOL_SOCKET, NN_RCVBUF, &value, sizeof(value)) < 0) {
        throw SocketError(tools::ToString("failed setting NN_RCVBUF, reason: ", nn_strerror(errno)));
    }
}

int FairMQSocketNN::GetRcvKernelSize() const
{
    int value = 0;
    size_t valueSize;
    if (nn_getsockopt(fSocket, NN_SOL_SOCKET, NN_RCVBUF, &value, &valueSize) < 0) {
        throw SocketError(tools::ToString("failed getting NN_RCVBUF, reason: ", nn_strerror(errno)));
    }
    return value;
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
        LOG(error) << "Multipart messages functionality currently not supported by nanomsg!";
        return -1;
    }
    if (constant == "rcv-more")
    {
        LOG(error) << "Multipart messages functionality currently not supported by nanomsg!";
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
