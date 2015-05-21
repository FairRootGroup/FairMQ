/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQChannel.cxx
 *
 * @since 2015-06-02
 * @author A. Rybalchenko
 */

#include <set>

#include "FairMQChannel.h"
#include "FairMQLogger.h"

using namespace std;

FairMQChannel::FairMQChannel()
    : fType("unspecified")
    , fMethod("unspecified")
    , fProtocol("unspecified")
    , fAddress("unspecified")
    , fPort("unspecified")
    , fSndBufSize(1000)
    , fRcvBufSize(1000)
    , fRateLogging(1)
    , fSocket()
    , fIsValid(false)
    , fChannelName("")
{
}

FairMQChannel::FairMQChannel(string type, string method, string protocol, string address, string port)
    : fType(type)
    , fMethod(method)
    , fProtocol(protocol)
    , fAddress(address)
    , fPort(port)
    , fSndBufSize(1000)
    , fRcvBufSize(1000)
    , fRateLogging(1)
    , fSocket()
    , fIsValid(false)
    , fChannelName("")
{
}

bool FairMQChannel::ValidateChannel()
{
    LOG(DEBUG) << "Validating channel " << fChannelName << "... ";
    if (fIsValid)
    {
        LOG(DEBUG) << "Channel is already valid";
        return true;
    }

    const string socketTypeNames[] = { "sub", "pub", "pull", "push", "req", "rep", "xsub", "xpub", "dealer", "router", "pair" };
    const set<string> socketTypes(socketTypeNames, socketTypeNames + sizeof(socketTypeNames) / sizeof(string));
    if (socketTypes.find(fType) == socketTypes.end())
    {
        LOG(DEBUG) << "Invalid channel type: " << fType;
        fIsValid = false;
        return false;
    }

    const string socketMethodNames[] = { "bind", "connect" };
    const set<string> socketMethods(socketMethodNames, socketMethodNames + sizeof(socketMethodNames) / sizeof(string));
    if (socketMethods.find(fMethod) == socketMethods.end())
    {
        LOG(DEBUG) << "Invalid channel method: " << fMethod;
        fIsValid = false;
        return false;
    }

    const string socketProtocolNames[] = { "tcp", "ipc", "inproc" };
    const set<string> socketProtocols(socketProtocolNames, socketProtocolNames + sizeof(socketProtocolNames) / sizeof(string));
    if (socketProtocols.find(fProtocol) == socketProtocols.end())
    {
        LOG(DEBUG) << "Invalid channel protocol: " << fProtocol;
        fIsValid = false;
        return false;
    }

    if (fAddress == "unspecified" && fAddress == "")
    {
        LOG(DEBUG) << "invalid channel address: " << fAddress;
        fIsValid = false;
        return false;
    }

    if (fPort == "unspecified" && fPort == "")
    {
        LOG(DEBUG) << "invalid channel port: " << fPort;
        fIsValid = false;
        return false;
    }

    if (fSndBufSize < 0)
    {
        LOG(DEBUG) << "invalid channel send buffer size: " << fSndBufSize;
        fIsValid = false;
        return false;
    }

    if (fRcvBufSize < 0)
    {
        LOG(DEBUG) << "invalid channel receive buffer size: " << fRcvBufSize;
        fIsValid = false;
        return false;
    }

    LOG(DEBUG) << "Channel is valid";
    fIsValid = true;
    return true;
}

int FairMQChannel::Send(FairMQMessage* msg, const string& flag)
{
    return fSocket->Send(msg, flag);
}

int FairMQChannel::Send(FairMQMessage* msg, const int flags)
{
    return fSocket->Send(msg, flags);
}

int FairMQChannel::Receive(FairMQMessage* msg, const string& flag)
{
    return fSocket->Receive(msg, flag);
}

int FairMQChannel::Receive(FairMQMessage* msg, const int flags)
{
    return fSocket->Receive(msg, flags);
}

FairMQChannel::~FairMQChannel()
{
}
