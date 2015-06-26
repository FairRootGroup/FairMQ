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

#include <boost/exception/all.hpp>

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
    try
    {
        boost::unique_lock<boost::mutex> scoped_lock(channelMutex);
        return fType;
    }
    catch (boost::exception& e)
    {
        LOG(ERROR) << boost::diagnostic_information(e);
    }
}

std::string FairMQChannel::GetMethod()
{
    try
    {
        boost::unique_lock<boost::mutex> scoped_lock(channelMutex);
        return fMethod;
    }
    catch (boost::exception& e)
    {
        LOG(ERROR) << boost::diagnostic_information(e);
    }
}

std::string FairMQChannel::GetAddress()
{
    try
    {
        boost::unique_lock<boost::mutex> scoped_lock(channelMutex);
        return fAddress;
    }
    catch (boost::exception& e)
    {
        LOG(ERROR) << boost::diagnostic_information(e);
    }
}

int FairMQChannel::GetSndBufSize()
{
    try
    {
        boost::unique_lock<boost::mutex> scoped_lock(channelMutex);
        return fSndBufSize;
    }
    catch (boost::exception& e)
    {
        LOG(ERROR) << boost::diagnostic_information(e);
    }
}

int FairMQChannel::GetRcvBufSize()
{
    try
    {
        boost::unique_lock<boost::mutex> scoped_lock(channelMutex);
        return fRcvBufSize;
    }
    catch (boost::exception& e)
    {
        LOG(ERROR) << boost::diagnostic_information(e);
    }
}

int FairMQChannel::GetRateLogging()
{
    try
    {
        boost::unique_lock<boost::mutex> scoped_lock(channelMutex);
        return fRateLogging;
    }
    catch (boost::exception& e)
    {
        LOG(ERROR) << boost::diagnostic_information(e);
    }
}

void FairMQChannel::UpdateType(const std::string& type)
{
    try
    {
        boost::unique_lock<boost::mutex> scoped_lock(channelMutex);
        fType = type;
    }
    catch (boost::exception& e)
    {
        LOG(ERROR) << boost::diagnostic_information(e);
    }
}
void FairMQChannel::UpdateMethod(const std::string& method)
{
    try
    {
        boost::unique_lock<boost::mutex> scoped_lock(channelMutex);
        fMethod = method;
    }
    catch (boost::exception& e)
    {
        LOG(ERROR) << boost::diagnostic_information(e);
    }
}
void FairMQChannel::UpdateAddress(const std::string& address)
{
    try
    {
        boost::unique_lock<boost::mutex> scoped_lock(channelMutex);
        fAddress = address;
    }
    catch (boost::exception& e)
    {
        LOG(ERROR) << boost::diagnostic_information(e);
    }
}
void FairMQChannel::UpdateSndBufSize(const int sndBufSize)
{
    try
    {
        boost::unique_lock<boost::mutex> scoped_lock(channelMutex);
        fSndBufSize = sndBufSize;
    }
    catch (boost::exception& e)
    {
        LOG(ERROR) << boost::diagnostic_information(e);
    }
}
void FairMQChannel::UpdateRcvBufSize(const int rcvBufSize)
{
    try
    {
        boost::unique_lock<boost::mutex> scoped_lock(channelMutex);
        fRcvBufSize = rcvBufSize;
    }
    catch (boost::exception& e)
    {
        LOG(ERROR) << boost::diagnostic_information(e);
    }
}
void FairMQChannel::UpdateRateLogging(const int rateLogging)
{
    try
    {
        boost::unique_lock<boost::mutex> scoped_lock(channelMutex);
        fRateLogging = rateLogging;
    }
    catch (boost::exception& e)
    {
        LOG(ERROR) << boost::diagnostic_information(e);
    }
}

bool FairMQChannel::IsValid()
{
    try
    {
        boost::unique_lock<boost::mutex> scoped_lock(channelMutex);
        return fIsValid;
    }
    catch (boost::exception& e)
    {
        LOG(ERROR) << boost::diagnostic_information(e);
    }
}

bool FairMQChannel::ValidateChannel()
{
    try
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

        if (fAddress == "unspecified" || fAddress == "")
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
    catch (boost::exception& e)
    {
        LOG(ERROR) << boost::diagnostic_information(e);
    }
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
