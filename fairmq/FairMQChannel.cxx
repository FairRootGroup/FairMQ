/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQChannel.cxx
 *
 * @since 2015-06-02
 * @author A. Rybalchenko
 */

#include "FairMQChannel.h"

#include <boost/algorithm/string.hpp> // join/split

#include <set>
#include <utility> // std::move

using namespace std;

mutex FairMQChannel::fChannelMutex;

FairMQChannel::FairMQChannel()
    : fSocket(nullptr)
    , fType("unspecified")
    , fMethod("unspecified")
    , fAddress("unspecified")
    , fTransportType(fair::mq::Transport::DEFAULT)
    , fSndBufSize(1000)
    , fRcvBufSize(1000)
    , fSndKernelSize(0)
    , fRcvKernelSize(0)
    , fLinger(500)
    , fRateLogging(1)
    , fName("")
    , fIsValid(false)
    , fTransportFactory(nullptr)
    , fMultipart(false)
    , fModified(true)
    , fReset(false)
{
}

FairMQChannel::FairMQChannel(const string& type, const string& method, const string& address)
    : fSocket(nullptr)
    , fType(type)
    , fMethod(method)
    , fAddress(address)
    , fTransportType(fair::mq::Transport::DEFAULT)
    , fSndBufSize(1000)
    , fRcvBufSize(1000)
    , fSndKernelSize(0)
    , fRcvKernelSize(0)
    , fLinger(500)
    , fRateLogging(1)
    , fName("")
    , fIsValid(false)
    , fTransportFactory(nullptr)
    , fMultipart(false)
    , fModified(true)
    , fReset(false)
{
}

FairMQChannel::FairMQChannel(const string& name, const string& type, shared_ptr<FairMQTransportFactory> factory)
    : fSocket(factory->CreateSocket(type, name))
    , fType(type)
    , fMethod("unspecified")
    , fAddress("unspecified")
    , fTransportType(factory->GetType())
    , fSndBufSize(1000)
    , fRcvBufSize(1000)
    , fSndKernelSize(0)
    , fRcvKernelSize(0)
    , fLinger(500)
    , fRateLogging(1)
    , fName(name)
    , fIsValid(false)
    , fTransportFactory(factory)
    , fMultipart(false)
    , fModified(true)
    , fReset(false)
{
}

FairMQChannel::FairMQChannel(const FairMQChannel& chan)
    : fSocket(nullptr)
    , fType(chan.fType)
    , fMethod(chan.fMethod)
    , fAddress(chan.fAddress)
    , fTransportType(chan.fTransportType)
    , fSndBufSize(chan.fSndBufSize)
    , fRcvBufSize(chan.fRcvBufSize)
    , fSndKernelSize(chan.fSndKernelSize)
    , fRcvKernelSize(chan.fRcvKernelSize)
    , fLinger(chan.fLinger)
    , fRateLogging(chan.fRateLogging)
    , fName(chan.fName)
    , fIsValid(false)
    , fTransportFactory(nullptr)
    , fMultipart(chan.fMultipart)
    , fModified(chan.fModified)
    , fReset(false)
{}

FairMQChannel& FairMQChannel::operator=(const FairMQChannel& chan)
{
    fSocket = nullptr;
    fType = chan.fType;
    fMethod = chan.fMethod;
    fAddress = chan.fAddress;
    fTransportType = chan.fTransportType;
    fSndBufSize = chan.fSndBufSize;
    fRcvBufSize = chan.fRcvBufSize;
    fSndKernelSize = chan.fSndKernelSize;
    fRcvKernelSize = chan.fRcvKernelSize;
    fLinger = chan.fLinger;
    fRateLogging = chan.fRateLogging;
    fName = chan.fName;
    fIsValid = false;
    fTransportFactory = nullptr;
    fMultipart = chan.fMultipart;
    fModified = chan.fModified;
    fReset = false;

    return *this;
}

FairMQSocket & FairMQChannel::GetSocket() const
{
    assert(fSocket);
    return *fSocket;
}

string FairMQChannel::GetChannelName() const
{
    return fName;
}

string FairMQChannel::GetChannelPrefix() const
{
    string prefix = fName;
    prefix = prefix.erase(fName.rfind("["));
    return prefix;
}

string FairMQChannel::GetChannelIndex() const
{
    string indexStr = fName;
    indexStr.erase(indexStr.rfind("]"));
    indexStr.erase(0, indexStr.rfind("[") + 1);
    return indexStr;
}

string FairMQChannel::GetType() const
{
    try
    {
        unique_lock<mutex> lock(fChannelMutex);
        return fType;
    }
    catch (exception& e)
    {
        LOG(error) << "Exception caught in FairMQChannel::GetType: " << e.what();
        exit(EXIT_FAILURE);
    }
}

string FairMQChannel::GetMethod() const
{
    try
    {
        unique_lock<mutex> lock(fChannelMutex);
        return fMethod;
    }
    catch (exception& e)
    {
        LOG(error) << "Exception caught in FairMQChannel::GetMethod: " << e.what();
        exit(EXIT_FAILURE);
    }
}

string FairMQChannel::GetAddress() const
{
    try
    {
        unique_lock<mutex> lock(fChannelMutex);
        return fAddress;
    }
    catch (exception& e)
    {
        LOG(error) << "Exception caught in FairMQChannel::GetAddress: " << e.what();
        exit(EXIT_FAILURE);
    }
}

string FairMQChannel::GetTransportName() const
{
    try
    {
        unique_lock<mutex> lock(fChannelMutex);
        return fair::mq::TransportNames.at(fTransportType);
    }
    catch (exception& e)
    {
        LOG(error) << "Exception caught in FairMQChannel::GetTransportName: " << e.what();
        exit(EXIT_FAILURE);
    }
}

int FairMQChannel::GetSndBufSize() const
{
    try
    {
        unique_lock<mutex> lock(fChannelMutex);
        return fSndBufSize;
    }
    catch (exception& e)
    {
        LOG(error) << "Exception caught in FairMQChannel::GetSndBufSize: " << e.what();
        exit(EXIT_FAILURE);
    }
}

int FairMQChannel::GetRcvBufSize() const
{
    try
    {
        unique_lock<mutex> lock(fChannelMutex);
        return fRcvBufSize;
    }
    catch (exception& e)
    {
        LOG(error) << "Exception caught in FairMQChannel::GetRcvBufSize: " << e.what();
        exit(EXIT_FAILURE);
    }
}

int FairMQChannel::GetSndKernelSize() const
{
    try
    {
        unique_lock<mutex> lock(fChannelMutex);
        return fSndKernelSize;
    }
    catch (exception& e)
    {
        LOG(error) << "Exception caught in FairMQChannel::GetSndKernelSize: " << e.what();
        exit(EXIT_FAILURE);
    }
}

int FairMQChannel::GetRcvKernelSize() const
{
    try
    {
        unique_lock<mutex> lock(fChannelMutex);
        return fRcvKernelSize;
    }
    catch (exception& e)
    {
        LOG(error) << "Exception caught in FairMQChannel::GetRcvKernelSize: " << e.what();
        exit(EXIT_FAILURE);
    }
}

int FairMQChannel::GetLinger() const
{
    try
    {
        unique_lock<mutex> lock(fChannelMutex);
        return fLinger;
    }
    catch (exception& e)
    {
        LOG(error) << "Exception caught in FairMQChannel::GetLinger: " << e.what();
        exit(EXIT_FAILURE);
    }
}

int FairMQChannel::GetRateLogging() const
{
    try
    {
        unique_lock<mutex> lock(fChannelMutex);
        return fRateLogging;
    }
    catch (exception& e)
    {
        LOG(error) << "Exception caught in FairMQChannel::GetRateLogging: " << e.what();
        exit(EXIT_FAILURE);
    }
}

void FairMQChannel::UpdateType(const string& type)
{
    try
    {
        unique_lock<mutex> lock(fChannelMutex);
        fIsValid = false;
        fType = type;
        fModified = true;
    }
    catch (exception& e)
    {
        LOG(error) << "Exception caught in FairMQChannel::UpdateType: " << e.what();
        exit(EXIT_FAILURE);
    }
}

void FairMQChannel::UpdateMethod(const string& method)
{
    try
    {
        unique_lock<mutex> lock(fChannelMutex);
        fIsValid = false;
        fMethod = method;
        fModified = true;
    }
    catch (exception& e)
    {
        LOG(error) << "Exception caught in FairMQChannel::UpdateMethod: " << e.what();
        exit(EXIT_FAILURE);
    }
}

void FairMQChannel::UpdateAddress(const string& address)
{
    try
    {
        unique_lock<mutex> lock(fChannelMutex);
        fIsValid = false;
        fAddress = address;
        fModified = true;
    }
    catch (exception& e)
    {
        LOG(error) << "Exception caught in FairMQChannel::UpdateAddress: " << e.what();
        exit(EXIT_FAILURE);
    }
}

void FairMQChannel::UpdateTransport(const string& transport)
{
    try
    {
        unique_lock<mutex> lock(fChannelMutex);
        fIsValid = false;
        fTransportType = fair::mq::TransportTypes.at(transport);
        fModified = true;
    }
    catch (exception& e)
    {
        LOG(error) << "Exception caught in FairMQChannel::UpdateTransport: " << e.what();
        exit(EXIT_FAILURE);
    }
}

void FairMQChannel::UpdateSndBufSize(const int sndBufSize)
{
    try
    {
        unique_lock<mutex> lock(fChannelMutex);
        fIsValid = false;
        fSndBufSize = sndBufSize;
        fModified = true;
    }
    catch (exception& e)
    {
        LOG(error) << "Exception caught in FairMQChannel::UpdateSndBufSize: " << e.what();
        exit(EXIT_FAILURE);
    }
}

void FairMQChannel::UpdateRcvBufSize(const int rcvBufSize)
{
    try
    {
        unique_lock<mutex> lock(fChannelMutex);
        fIsValid = false;
        fRcvBufSize = rcvBufSize;
        fModified = true;
    }
    catch (exception& e)
    {
        LOG(error) << "Exception caught in FairMQChannel::UpdateRcvBufSize: " << e.what();
        exit(EXIT_FAILURE);
    }
}

void FairMQChannel::UpdateSndKernelSize(const int sndKernelSize)
{
    try
    {
        unique_lock<mutex> lock(fChannelMutex);
        fIsValid = false;
        fSndKernelSize = sndKernelSize;
        fModified = true;
    }
    catch (exception& e)
    {
        LOG(error) << "Exception caught in FairMQChannel::UpdateSndKernelSize: " << e.what();
        exit(EXIT_FAILURE);
    }
}

void FairMQChannel::UpdateRcvKernelSize(const int rcvKernelSize)
{
    try
    {
        unique_lock<mutex> lock(fChannelMutex);
        fIsValid = false;
        fRcvKernelSize = rcvKernelSize;
        fModified = true;
    }
    catch (exception& e)
    {
        LOG(error) << "Exception caught in FairMQChannel::UpdateRcvKernelSize: " << e.what();
        exit(EXIT_FAILURE);
    }
}

void FairMQChannel::UpdateLinger(const int duration)
{
    try
    {
        unique_lock<mutex> lock(fChannelMutex);
        fIsValid = false;
        fLinger = duration;
        fModified = true;
    }
    catch (exception& e)
    {
        LOG(error) << "Exception caught in FairMQChannel::UpdateLinger: " << e.what();
        exit(EXIT_FAILURE);
    }
}

void FairMQChannel::UpdateRateLogging(const int rateLogging)
{
    try
    {
        unique_lock<mutex> lock(fChannelMutex);
        fIsValid = false;
        fRateLogging = rateLogging;
        fModified = true;
    }
    catch (exception& e)
    {
        LOG(error) << "Exception caught in FairMQChannel::UpdateRateLogging: " << e.what();
        exit(EXIT_FAILURE);
    }
}

auto FairMQChannel::SetModified(const bool modified) -> void
{
    try
    {
        unique_lock<mutex> lock(fChannelMutex);
        fModified = modified;
    }
    catch (exception& e)
    {
        LOG(error) << "Exception caught in FairMQChannel::SetModified: " << e.what();
        exit(EXIT_FAILURE);
    }
}

void FairMQChannel::UpdateChannelName(const string& name)
{
    try
    {
        unique_lock<mutex> lock(fChannelMutex);
        fIsValid = false;
        fName = name;
        fModified = true;
    }
    catch (exception& e)
    {
        LOG(error) << "Exception caught in FairMQChannel::UpdateChannelName: " << e.what();
        exit(EXIT_FAILURE);
    }
}

bool FairMQChannel::IsValid() const
{
    try
    {
        unique_lock<mutex> lock(fChannelMutex);
        return fIsValid;
    }
    catch (exception& e)
    {
        LOG(error) << "Exception caught in FairMQChannel::IsValid: " << e.what();
        exit(EXIT_FAILURE);
    }
}

bool FairMQChannel::ValidateChannel()
{
    try
    {
        unique_lock<mutex> lock(fChannelMutex);

        stringstream ss;
        ss << "Validating channel \"" << fName << "\"... ";

        if (fIsValid)
        {
            ss << "ALREADY VALID";
            LOG(debug) << ss.str();
            return true;
        }

        // validate socket type
        const string socketTypeNames[] = { "sub", "pub", "pull", "push", "req", "rep", "xsub", "xpub", "dealer", "router", "pair" };
        const set<string> socketTypes(socketTypeNames, socketTypeNames + sizeof(socketTypeNames) / sizeof(string));
        if (socketTypes.find(fType) == socketTypes.end())
        {
            ss << "INVALID";
            LOG(debug) << ss.str();
            LOG(error) << "Invalid channel type: \"" << fType << "\"";
            exit(EXIT_FAILURE);
        }

        // validate socket address
        if (fAddress == "unspecified" || fAddress == "")
        {
            ss << "INVALID";
            LOG(debug) << ss.str();
            LOG(debug) << "invalid channel address: \"" << fAddress << "\"";
            return false;
        }
        else
        {
            vector<string> endpoints;
            boost::algorithm::split(endpoints, fAddress, boost::algorithm::is_any_of(";"));
            for (const auto endpoint : endpoints)
            {
                string address;
                if (endpoint[0] == '@' || endpoint[0] == '+' || endpoint[0] == '>')
                {
                    address = endpoint.substr(1);
                }
                else
                {
                    // we don't have a method modifier, check if the default method is set
                    const string socketMethodNames[] = { "bind", "connect" };
                    const set<string> socketMethods(socketMethodNames, socketMethodNames + sizeof(socketMethodNames) / sizeof(string));
                    if (socketMethods.find(fMethod) == socketMethods.end())
                    {
                        ss << "INVALID";
                        LOG(debug) << ss.str();
                        LOG(error) << "Invalid endpoint connection method: \"" << fMethod << "\" for " << endpoint;
                        exit(EXIT_FAILURE);
                    }
                    address = endpoint;
                }
                // check if address is a tcp or ipc address
                if (address.compare(0, 6, "tcp://") == 0)
                {
                    // check if TCP address contains port delimiter
                    string addressString = address.substr(6);
                    if (addressString.find(":") == string::npos)
                    {
                        ss << "INVALID";
                        LOG(debug) << ss.str();
                        LOG(error) << "invalid channel address: \"" << address << "\" (missing port?)";
                        return false;
                    }
                }
                else if (address.compare(0, 6, "ipc://") == 0)
                {
                    // check if IPC address is not empty
                    string addressString = address.substr(6);
                    if (addressString == "")
                    {
                        ss << "INVALID";
                        LOG(debug) << ss.str();
                        LOG(error) << "invalid channel address: \"" << address << "\" (empty IPC address?)";
                        return false;
                    }
                }
                else if (address.compare(0, 9, "inproc://") == 0)
                {
                    // check if IPC address is not empty
                    string addressString = address.substr(9);
                    if (addressString == "")
                    {
                        ss << "INVALID";
                        LOG(debug) << ss.str();
                        LOG(error) << "invalid channel address: \"" << address << "\" (empty inproc address?)";
                        return false;
                    }
                }
                else if (address.compare(0, 8, "verbs://") == 0)
                {
                    // check if IPC address is not empty
                    string addressString = address.substr(9);
                    if (addressString == "")
                    {
                        ss << "INVALID";
                        LOG(debug) << ss.str();
                        LOG(error) << "invalid channel address: \"" << address << "\" (empty verbs address?)";
                        return false;
                    }
                }
                else
                {
                    // if neither TCP or IPC is specified, return invalid
                    ss << "INVALID";
                    LOG(debug) << ss.str();
                    LOG(error) << "invalid channel address: \"" << address << "\" (missing protocol specifier?)";
                    return false;
                }
            }
        }

        // validate socket buffer size for sending
        if (fSndBufSize < 0)
        {
            ss << "INVALID";
            LOG(debug) << ss.str();
            LOG(error) << "invalid channel send buffer size (cannot be negative): \"" << fSndBufSize << "\"";
            exit(EXIT_FAILURE);
        }

        // validate socket buffer size for receiving
        if (fRcvBufSize < 0)
        {
            ss << "INVALID";
            LOG(debug) << ss.str();
            LOG(error) << "invalid channel receive buffer size (cannot be negative): \"" << fRcvBufSize << "\"";
            exit(EXIT_FAILURE);
        }

        // validate socket kernel transmit size for sending
        if (fSndKernelSize < 0)
        {
            ss << "INVALID";
            LOG(debug) << ss.str();
            LOG(error) << "invalid channel send kernel transmit size (cannot be negative): \"" << fSndKernelSize << "\"";
            exit(EXIT_FAILURE);
        }

        // validate socket kernel transmit size for receiving
        if (fRcvKernelSize < 0)
        {
            ss << "INVALID";
            LOG(debug) << ss.str();
            LOG(error) << "invalid channel receive kernel transmit size (cannot be negative): \"" << fRcvKernelSize << "\"";
            exit(EXIT_FAILURE);
        }

        // validate socket rate logging interval
        if (fRateLogging < 0)
        {
            ss << "INVALID";
            LOG(debug) << ss.str();
            LOG(error) << "invalid socket rate logging interval (cannot be negative): \"" << fRateLogging << "\"";
            exit(EXIT_FAILURE);
        }

        fIsValid = true;
        ss << "VALID";
        LOG(debug) << ss.str();
        return true;
    }
    catch (exception& e)
    {
        LOG(error) << "Exception caught in FairMQChannel::ValidateChannel: " << e.what();
        exit(EXIT_FAILURE);
    }
}

void FairMQChannel::InitTransport(shared_ptr<FairMQTransportFactory> factory)
{
    fTransportFactory = factory;
    fTransportType = factory->GetType();
}

void FairMQChannel::ResetChannel()
{
    unique_lock<mutex> lock(fChannelMutex);
    fIsValid = false;
    // TODO: implement channel resetting
}

int FairMQChannel::Send(unique_ptr<FairMQMessage>& msg) const
{
    CheckSendCompatibility(msg);
    return fSocket->Send(msg);
}

int FairMQChannel::Receive(unique_ptr<FairMQMessage>& msg) const
{
    CheckReceiveCompatibility(msg);
    return fSocket->Receive(msg);
}

int FairMQChannel::Send(unique_ptr<FairMQMessage>& msg, int sndTimeoutInMs) const
{
    return fSocket->Send(msg, sndTimeoutInMs);
}

int FairMQChannel::Receive(unique_ptr<FairMQMessage>& msg, int rcvTimeoutInMs) const
{
    return fSocket->Receive(msg, rcvTimeoutInMs);
}

int FairMQChannel::SendAsync(unique_ptr<FairMQMessage>& msg) const
{
    CheckSendCompatibility(msg);
    return fSocket->TrySend(msg);
}

int FairMQChannel::ReceiveAsync(unique_ptr<FairMQMessage>& msg) const
{
    CheckReceiveCompatibility(msg);
    return fSocket->TryReceive(msg);
}

int64_t FairMQChannel::Send(vector<unique_ptr<FairMQMessage>>& msgVec) const
{
    CheckSendCompatibility(msgVec);
    return fSocket->Send(msgVec);
}

int64_t FairMQChannel::Receive(vector<unique_ptr<FairMQMessage>>& msgVec) const
{
    CheckReceiveCompatibility(msgVec);
    return fSocket->Receive(msgVec);
}

int64_t FairMQChannel::Send(vector<unique_ptr<FairMQMessage>>& msgVec, int sndTimeoutInMs) const
{
    return fSocket->Send(msgVec, sndTimeoutInMs);
}

int64_t FairMQChannel::Receive(vector<unique_ptr<FairMQMessage>>& msgVec, int rcvTimeoutInMs) const
{
    return fSocket->Receive(msgVec, rcvTimeoutInMs);
}

int64_t FairMQChannel::SendAsync(vector<unique_ptr<FairMQMessage>>& msgVec) const
{
    CheckSendCompatibility(msgVec);
    return fSocket->TrySend(msgVec);
}

/// Receives a vector of messages in non-blocking mode.
///
/// @param msgVec message vector reference
/// @return Number of bytes that have been received. If queue is empty, returns -2.
/// In case of errors, returns -1.
int64_t FairMQChannel::ReceiveAsync(vector<unique_ptr<FairMQMessage>>& msgVec) const
{
    CheckReceiveCompatibility(msgVec);
    return fSocket->TryReceive(msgVec);
}

FairMQChannel::~FairMQChannel()
{
}

unsigned long FairMQChannel::GetBytesTx() const
{
    return fSocket->GetBytesTx();
}

unsigned long FairMQChannel::GetBytesRx() const
{
    return fSocket->GetBytesRx();
}

unsigned long FairMQChannel::GetMessagesTx() const
{
    return fSocket->GetMessagesTx();
}

unsigned long FairMQChannel::GetMessagesRx() const
{
    return fSocket->GetMessagesRx();
}

void FairMQChannel::CheckSendCompatibility(FairMQMessagePtr& msg) const
{
    if (fTransportType != msg->GetType())
    {
        // LOG(debug) << "Channel type does not match message type. Creating wrapper";
        FairMQMessagePtr msgWrapper(NewMessage(msg->GetData(),
                                               msg->GetSize(),
                                               [](void* /*data*/, void* _msg) { delete static_cast<FairMQMessage*>(_msg); },
                                               msg.get()
                                               ));
        msg.release();
        msg = move(msgWrapper);
    }
}

void FairMQChannel::CheckSendCompatibility(vector<FairMQMessagePtr>& msgVec) const
{
    for (auto& msg : msgVec)
    {
        if (fTransportType != msg->GetType())
        {
            // LOG(debug) << "Channel type does not match message type. Creating wrapper";
            FairMQMessagePtr msgWrapper(NewMessage(msg->GetData(),
                                                   msg->GetSize(),
                                                   [](void* /*data*/, void* _msg) { delete static_cast<FairMQMessage*>(_msg); },
                                                   msg.get()
                                                   ));
            msg.release();
            msg = move(msgWrapper);
        }
    }
}

void FairMQChannel::CheckReceiveCompatibility(FairMQMessagePtr& msg) const
{
    if (fTransportType != msg->GetType())
    {
        // LOG(debug) << "Channel type does not match message type. Creating wrapper";
        FairMQMessagePtr newMsg(NewMessage());
        msg = move(newMsg);
    }
}

void FairMQChannel::CheckReceiveCompatibility(vector<FairMQMessagePtr>& msgVec) const
{
    for (auto& msg : msgVec)
    {
        if (fTransportType != msg->GetType())
        {
            // LOG(debug) << "Channel type does not match message type. Creating wrapper";
            FairMQMessagePtr newMsg(NewMessage());
            msg = move(newMsg);
        }
    }
}
