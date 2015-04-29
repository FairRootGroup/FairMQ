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
    , fAddress("unspecified")
    , fSndBufSize(1000)
    , fRcvBufSize(1000)
    , fRateLogging(1)
    , fSocket()
    , fChannelName("")
{
}

FairMQChannel::FairMQChannel(const string& type, const string& method, const string& address)
    : fType(type)
    , fMethod(method)
    , fAddress(address)
    , fSndBufSize(1000)
    , fRcvBufSize(1000)
    , fRateLogging(1)
    , fSocket()
    , fChannelName("")
{
}

bool FairMQChannel::ValidateChannel()
{
    stringstream ss;
    ss << "Validating channel " << fChannelName << "... ";

    const string socketTypeNames[] = { "sub", "pub", "pull", "push", "req", "rep", "xsub", "xpub", "dealer", "router", "pair" };
    const set<string> socketTypes(socketTypeNames, socketTypeNames + sizeof(socketTypeNames) / sizeof(string));
    if (socketTypes.find(fType) == socketTypes.end())
    {
        ss << "INVALID";
        LOG(DEBUG) << ss.str();
        LOG(DEBUG) << "Invalid channel type: " << fType;
        return false;
    }

    const string socketMethodNames[] = { "bind", "connect" };
    const set<string> socketMethods(socketMethodNames, socketMethodNames + sizeof(socketMethodNames) / sizeof(string));
    if (socketMethods.find(fMethod) == socketMethods.end())
    {
        ss << "INVALID";
        LOG(DEBUG) << ss.str();
        LOG(DEBUG) << "Invalid channel method: " << fMethod;
        return false;
    }

    if (fAddress == "unspecified" && fAddress == "")
    {
        ss << "INVALID";
        LOG(DEBUG) << ss.str();
        LOG(DEBUG) << "invalid channel address: " << fAddress;
        return false;
    }

    if (fSndBufSize < 0)
    {
        ss << "INVALID";
        LOG(DEBUG) << ss.str();
        LOG(DEBUG) << "invalid channel send buffer size: " << fSndBufSize;
        return false;
    }

    if (fRcvBufSize < 0)
    {
        ss << "INVALID";
        LOG(DEBUG) << ss.str();
        LOG(DEBUG) << "invalid channel receive buffer size: " << fRcvBufSize;
        return false;
    }

    ss << "VALID";
    LOG(DEBUG) << ss.str();
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
