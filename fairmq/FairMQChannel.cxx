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

boost::mutex FairMQChannel::channelMutex;

FairMQChannel::FairMQChannel()
    : fType("unspecified")
    , fMethod("unspecified")
    , fAddress("unspecified")
    , fSndBufSize(1000)
    , fRcvBufSize(1000)
    , fRateLogging(1)
    , fSocket()
    , fChannelName("")
    , fIsValid(false)
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
    , fIsValid(false)
{
}

std::string FairMQChannel::GetType()
{
    boost::unique_lock<boost::mutex> scoped_lock(channelMutex);
    return fType;
}

std::string FairMQChannel::GetMethod()
{
    boost::unique_lock<boost::mutex> scoped_lock(channelMutex);
    return fMethod;
}

std::string FairMQChannel::GetAddress()
{
    boost::unique_lock<boost::mutex> scoped_lock(channelMutex);
    return fAddress;
}

int FairMQChannel::GetSndBufSize()
{
    boost::unique_lock<boost::mutex> scoped_lock(channelMutex);
    return fSndBufSize;
}

int FairMQChannel::GetRcvBufSize()
{
    boost::unique_lock<boost::mutex> scoped_lock(channelMutex);
    return fRcvBufSize;
}

int FairMQChannel::GetRateLogging()
{
    boost::unique_lock<boost::mutex> scoped_lock(channelMutex);
    return fRateLogging;
}

void FairMQChannel::UpdateType(const std::string& type)
{
    boost::unique_lock<boost::mutex> scoped_lock(channelMutex);
    fType = type;
}
void FairMQChannel::UpdateMethod(const std::string& method)
{
    boost::unique_lock<boost::mutex> scoped_lock(channelMutex);
    fMethod = method;
}
void FairMQChannel::UpdateAddress(const std::string& address)
{
    boost::unique_lock<boost::mutex> scoped_lock(channelMutex);
    fAddress = address;
}
void FairMQChannel::UpdateSndBufSize(const int sndBufSize)
{
    boost::unique_lock<boost::mutex> scoped_lock(channelMutex);
    fSndBufSize = sndBufSize;
}
void FairMQChannel::UpdateRcvBufSize(const int rcvBufSize)
{
    boost::unique_lock<boost::mutex> scoped_lock(channelMutex);
    fRcvBufSize = rcvBufSize;
}
void FairMQChannel::UpdateRateLogging(const int rateLogging)
{
    boost::unique_lock<boost::mutex> scoped_lock(channelMutex);
    fRateLogging = rateLogging;
}

bool FairMQChannel::IsValid()
{
    boost::unique_lock<boost::mutex> scoped_lock(channelMutex);
    return fIsValid;
}

bool FairMQChannel::ValidateChannel()
{
    boost::unique_lock<boost::mutex> scoped_lock(channelMutex);

    stringstream ss;
    ss << "Validating channel " << fChannelName << "... ";

    if (fIsValid)
    {
        ss << "ALREADY VALID";
        LOG(DEBUG) << ss.str();
        return true;
    }

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

    fIsValid = true;
    ss << "VALID";
    LOG(DEBUG) << ss.str();
    return true;
}

void FairMQChannel::ResetChannel()
{
    // TODO: implement resetting
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
