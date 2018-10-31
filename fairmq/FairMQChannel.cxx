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
#include <fairmq/Tools.h>

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
    prefix = prefix.erase(fName.rfind('['));
    return prefix;
}

string FairMQChannel::GetChannelIndex() const
{
    string indexStr = fName;
    indexStr.erase(indexStr.rfind(']'));
    indexStr.erase(0, indexStr.rfind('[') + 1);
    return indexStr;
}

string FairMQChannel::GetType() const
try {
    lock_guard<mutex> lock(fChannelMutex);
    return fType;
} catch (exception& e) {
    LOG(error) << "Exception caught in FairMQChannel::GetType: " << e.what();
    throw ChannelConfigurationError(fair::mq::tools::ToString("failed to acquire lock: ", e.what()));
}

string FairMQChannel::GetMethod() const
try {
    lock_guard<mutex> lock(fChannelMutex);
    return fMethod;
} catch (exception& e) {
    LOG(error) << "Exception caught in FairMQChannel::GetMethod: " << e.what();
    throw ChannelConfigurationError(fair::mq::tools::ToString("failed to acquire lock: ", e.what()));
}

string FairMQChannel::GetAddress() const
try {
    lock_guard<mutex> lock(fChannelMutex);
    return fAddress;
} catch (exception& e) {
    LOG(error) << "Exception caught in FairMQChannel::GetAddress: " << e.what();
    throw ChannelConfigurationError(fair::mq::tools::ToString("failed to acquire lock: ", e.what()));
}

string FairMQChannel::GetTransportName() const
try {
    lock_guard<mutex> lock(fChannelMutex);
    return fair::mq::TransportNames.at(fTransportType);
} catch (exception& e) {
    LOG(error) << "Exception caught in FairMQChannel::GetTransportName: " << e.what();
    throw ChannelConfigurationError(fair::mq::tools::ToString("failed to acquire lock: ", e.what()));
}

int FairMQChannel::GetSndBufSize() const
try {
    lock_guard<mutex> lock(fChannelMutex);
    return fSndBufSize;
} catch (exception& e) {
    LOG(error) << "Exception caught in FairMQChannel::GetSndBufSize: " << e.what();
    throw ChannelConfigurationError(fair::mq::tools::ToString("failed to acquire lock: ", e.what()));
}

int FairMQChannel::GetRcvBufSize() const
try {
    lock_guard<mutex> lock(fChannelMutex);
    return fRcvBufSize;
} catch (exception& e) {
    LOG(error) << "Exception caught in FairMQChannel::GetRcvBufSize: " << e.what();
    throw ChannelConfigurationError(fair::mq::tools::ToString("failed to acquire lock: ", e.what()));
}

int FairMQChannel::GetSndKernelSize() const
try {
    lock_guard<mutex> lock(fChannelMutex);
    return fSndKernelSize;
} catch (exception& e) {
    LOG(error) << "Exception caught in FairMQChannel::GetSndKernelSize: " << e.what();
    throw ChannelConfigurationError(fair::mq::tools::ToString("failed to acquire lock: ", e.what()));
}

int FairMQChannel::GetRcvKernelSize() const
try {
    lock_guard<mutex> lock(fChannelMutex);
    return fRcvKernelSize;
} catch (exception& e) {
    LOG(error) << "Exception caught in FairMQChannel::GetRcvKernelSize: " << e.what();
    throw ChannelConfigurationError(fair::mq::tools::ToString("failed to acquire lock: ", e.what()));
}

int FairMQChannel::GetLinger() const
try {
    lock_guard<mutex> lock(fChannelMutex);
    return fLinger;
} catch (exception& e) {
    LOG(error) << "Exception caught in FairMQChannel::GetLinger: " << e.what();
    throw ChannelConfigurationError(fair::mq::tools::ToString("failed to acquire lock: ", e.what()));
}

int FairMQChannel::GetRateLogging() const
try {
    lock_guard<mutex> lock(fChannelMutex);
    return fRateLogging;
} catch (exception& e) {
    LOG(error) << "Exception caught in FairMQChannel::GetRateLogging: " << e.what();
    throw ChannelConfigurationError(fair::mq::tools::ToString("failed to acquire lock: ", e.what()));
}

void FairMQChannel::UpdateType(const string& type)
try {
    lock_guard<mutex> lock(fChannelMutex);
    fIsValid = false;
    fType = type;
    fModified = true;
} catch (exception& e) {
    LOG(error) << "Exception caught in FairMQChannel::UpdateType: " << e.what();
    throw ChannelConfigurationError(fair::mq::tools::ToString("failed to acquire lock: ", e.what()));
}

void FairMQChannel::UpdateMethod(const string& method)
try {
    lock_guard<mutex> lock(fChannelMutex);
    fIsValid = false;
    fMethod = method;
    fModified = true;
} catch (exception& e) {
    LOG(error) << "Exception caught in FairMQChannel::UpdateMethod: " << e.what();
    throw ChannelConfigurationError(fair::mq::tools::ToString("failed to acquire lock: ", e.what()));
}

void FairMQChannel::UpdateAddress(const string& address)
try {
    lock_guard<mutex> lock(fChannelMutex);
    fIsValid = false;
    fAddress = address;
    fModified = true;
} catch (exception& e) {
    LOG(error) << "Exception caught in FairMQChannel::UpdateAddress: " << e.what();
    throw ChannelConfigurationError(fair::mq::tools::ToString("failed to acquire lock: ", e.what()));
}

void FairMQChannel::UpdateTransport(const string& transport)
try {
    lock_guard<mutex> lock(fChannelMutex);
    fIsValid = false;
    fTransportType = fair::mq::TransportTypes.at(transport);
    fModified = true;
} catch (exception& e) {
    LOG(error) << "Exception caught in FairMQChannel::UpdateTransport: " << e.what();
    throw ChannelConfigurationError(fair::mq::tools::ToString("failed to acquire lock: ", e.what()));
}

void FairMQChannel::UpdateSndBufSize(const int sndBufSize)
try {
    lock_guard<mutex> lock(fChannelMutex);
    fIsValid = false;
    fSndBufSize = sndBufSize;
    fModified = true;
} catch (exception& e) {
    LOG(error) << "Exception caught in FairMQChannel::UpdateSndBufSize: " << e.what();
    throw ChannelConfigurationError(fair::mq::tools::ToString("failed to acquire lock: ", e.what()));
}

void FairMQChannel::UpdateRcvBufSize(const int rcvBufSize)
try {
    lock_guard<mutex> lock(fChannelMutex);
    fIsValid = false;
    fRcvBufSize = rcvBufSize;
    fModified = true;
} catch (exception& e) {
    LOG(error) << "Exception caught in FairMQChannel::UpdateRcvBufSize: " << e.what();
    throw ChannelConfigurationError(fair::mq::tools::ToString("failed to acquire lock: ", e.what()));
}

void FairMQChannel::UpdateSndKernelSize(const int sndKernelSize)
try {
    lock_guard<mutex> lock(fChannelMutex);
    fIsValid = false;
    fSndKernelSize = sndKernelSize;
    fModified = true;
} catch (exception& e) {
    LOG(error) << "Exception caught in FairMQChannel::UpdateSndKernelSize: " << e.what();
    throw ChannelConfigurationError(fair::mq::tools::ToString("failed to acquire lock: ", e.what()));
}

void FairMQChannel::UpdateRcvKernelSize(const int rcvKernelSize)
try {
    lock_guard<mutex> lock(fChannelMutex);
    fIsValid = false;
    fRcvKernelSize = rcvKernelSize;
    fModified = true;
} catch (exception& e) {
    LOG(error) << "Exception caught in FairMQChannel::UpdateRcvKernelSize: " << e.what();
    throw ChannelConfigurationError(fair::mq::tools::ToString("failed to acquire lock: ", e.what()));
}

void FairMQChannel::UpdateLinger(const int duration)
try {
    lock_guard<mutex> lock(fChannelMutex);
    fIsValid = false;
    fLinger = duration;
    fModified = true;
} catch (exception& e) {
    LOG(error) << "Exception caught in FairMQChannel::UpdateLinger: " << e.what();
    throw ChannelConfigurationError(fair::mq::tools::ToString("failed to acquire lock: ", e.what()));
}

void FairMQChannel::UpdateRateLogging(const int rateLogging)
try {
    lock_guard<mutex> lock(fChannelMutex);
    fIsValid = false;
    fRateLogging = rateLogging;
    fModified = true;
} catch (exception& e) {
    LOG(error) << "Exception caught in FairMQChannel::UpdateRateLogging: " << e.what();
    throw ChannelConfigurationError(fair::mq::tools::ToString("failed to acquire lock: ", e.what()));
}

auto FairMQChannel::SetModified(const bool modified) -> void
try {
    lock_guard<mutex> lock(fChannelMutex);
    fModified = modified;
} catch (exception& e) {
    LOG(error) << "Exception caught in FairMQChannel::SetModified: " << e.what();
    throw ChannelConfigurationError(fair::mq::tools::ToString("failed to acquire lock: ", e.what()));
}

void FairMQChannel::UpdateChannelName(const string& name)
try {
    lock_guard<mutex> lock(fChannelMutex);
    fIsValid = false;
    fName = name;
    fModified = true;
} catch (exception& e) {
    LOG(error) << "Exception caught in FairMQChannel::UpdateChannelName: " << e.what();
    throw ChannelConfigurationError(fair::mq::tools::ToString("failed to acquire lock: ", e.what()));
}

bool FairMQChannel::IsValid() const
try {
    lock_guard<mutex> lock(fChannelMutex);
    return fIsValid;
} catch (exception& e) {
    LOG(error) << "Exception caught in FairMQChannel::IsValid: " << e.what();
    throw ChannelConfigurationError(fair::mq::tools::ToString("failed to acquire lock: ", e.what()));
}

bool FairMQChannel::ValidateChannel()
try {
    lock_guard<mutex> lock(fChannelMutex);
    stringstream ss;
    ss << "Validating channel '" << fName << "'... ";

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
        LOG(error) << "Invalid channel type: '" << fType << "'";
        throw ChannelConfigurationError(fair::mq::tools::ToString("Invalid channel type: '", fType, "'"));
    }

    // validate socket address
    if (fAddress == "unspecified" || fAddress == "")
    {
        ss << "INVALID";
        LOG(debug) << ss.str();
        LOG(debug) << "invalid channel address: '" << fAddress << "'";
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
                    LOG(error) << "Invalid endpoint connection method: '" << fMethod << "' for " << endpoint;
                    throw ChannelConfigurationError(fair::mq::tools::ToString("Invalid endpoint connection method: '", fMethod, "' for ", endpoint));
                }
                address = endpoint;
            }
            // check if address is a tcp or ipc address
            if (address.compare(0, 6, "tcp://") == 0)
            {
                // check if TCP address contains port delimiter
                string addressString = address.substr(6);
                if (addressString.find(':') == string::npos)
                {
                    ss << "INVALID";
                    LOG(debug) << ss.str();
                    LOG(error) << "invalid channel address: '" << address << "' (missing port?)";
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
                    LOG(error) << "invalid channel address: '" << address << "' (empty IPC address?)";
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
                    LOG(error) << "invalid channel address: '" << address << "' (empty inproc address?)";
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
                    LOG(error) << "invalid channel address: '" << address << "' (empty verbs address?)";
                    return false;
                }
            }
            else
            {
                // if neither TCP or IPC is specified, return invalid
                ss << "INVALID";
                LOG(debug) << ss.str();
                LOG(error) << "invalid channel address: '" << address << "' (missing protocol specifier?)";
                return false;
            }
        }
    }

    // validate socket buffer size for sending
    if (fSndBufSize < 0)
    {
        ss << "INVALID";
        LOG(debug) << ss.str();
        LOG(error) << "invalid channel send buffer size (cannot be negative): '" << fSndBufSize << "'";
        throw ChannelConfigurationError(fair::mq::tools::ToString("invalid channel send buffer size (cannot be negative): '", fSndBufSize, "'"));
    }

    // validate socket buffer size for receiving
    if (fRcvBufSize < 0)
    {
        ss << "INVALID";
        LOG(debug) << ss.str();
        LOG(error) << "invalid channel receive buffer size (cannot be negative): '" << fRcvBufSize << "'";
        throw ChannelConfigurationError(fair::mq::tools::ToString("invalid channel receive buffer size (cannot be negative): '", fRcvBufSize, "'"));
    }

    // validate socket kernel transmit size for sending
    if (fSndKernelSize < 0)
    {
        ss << "INVALID";
        LOG(debug) << ss.str();
        LOG(error) << "invalid channel send kernel transmit size (cannot be negative): '" << fSndKernelSize << "'";
        throw ChannelConfigurationError(fair::mq::tools::ToString("invalid channel send kernel transmit size (cannot be negative): '", fSndKernelSize, "'"));
    }

    // validate socket kernel transmit size for receiving
    if (fRcvKernelSize < 0)
    {
        ss << "INVALID";
        LOG(debug) << ss.str();
        LOG(error) << "invalid channel receive kernel transmit size (cannot be negative): '" << fRcvKernelSize << "'";
        throw ChannelConfigurationError(fair::mq::tools::ToString("invalid channel receive kernel transmit size (cannot be negative): '", fRcvKernelSize, "'"));
    }

    // validate socket rate logging interval
    if (fRateLogging < 0)
    {
        ss << "INVALID";
        LOG(debug) << ss.str();
        LOG(error) << "invalid socket rate logging interval (cannot be negative): '" << fRateLogging << "'";
        throw ChannelConfigurationError(fair::mq::tools::ToString("invalid socket rate logging interval (cannot be negative): '", fRateLogging, "'"));
    }

    fIsValid = true;
    ss << "VALID";
    LOG(debug) << ss.str();
    return true;
} catch (exception& e) {
    LOG(error) << "Exception caught in FairMQChannel::ValidateChannel: " << e.what();
    throw ChannelConfigurationError(fair::mq::tools::ToString(e.what()));
}

void FairMQChannel::InitTransport(shared_ptr<FairMQTransportFactory> factory)
{
    fTransportFactory = factory;
    fTransportType = factory->GetType();
}

void FairMQChannel::ResetChannel()
{
    lock_guard<mutex> lock(fChannelMutex);
    fIsValid = false;
    // TODO: implement channel resetting
}

int FairMQChannel::Send(unique_ptr<FairMQMessage>& msg, int sndTimeoutInMs)
{
    CheckSendCompatibility(msg);
    return fSocket->Send(msg, sndTimeoutInMs);
}

int FairMQChannel::Receive(unique_ptr<FairMQMessage>& msg, int rcvTimeoutInMs)
{
    CheckReceiveCompatibility(msg);
    return fSocket->Receive(msg, rcvTimeoutInMs);
}

int FairMQChannel::SendAsync(unique_ptr<FairMQMessage>& msg)
{
    CheckSendCompatibility(msg);
    return fSocket->Send(msg, 0);
}

int FairMQChannel::ReceiveAsync(unique_ptr<FairMQMessage>& msg)
{
    CheckReceiveCompatibility(msg);
    return fSocket->Receive(msg, 0);
}

int64_t FairMQChannel::Send(vector<unique_ptr<FairMQMessage>>& msgVec, int sndTimeoutInMs)
{
    CheckSendCompatibility(msgVec);
    return fSocket->Send(msgVec, sndTimeoutInMs);
}

int64_t FairMQChannel::Receive(vector<unique_ptr<FairMQMessage>>& msgVec, int rcvTimeoutInMs)
{
    CheckReceiveCompatibility(msgVec);
    return fSocket->Receive(msgVec, rcvTimeoutInMs);
}

int64_t FairMQChannel::SendAsync(vector<unique_ptr<FairMQMessage>>& msgVec)
{
    CheckSendCompatibility(msgVec);
    return fSocket->Send(msgVec, 0);
}

int64_t FairMQChannel::ReceiveAsync(vector<unique_ptr<FairMQMessage>>& msgVec)
{
    CheckReceiveCompatibility(msgVec);
    return fSocket->Receive(msgVec, 0);
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

void FairMQChannel::CheckSendCompatibility(FairMQMessagePtr& msg)
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

void FairMQChannel::CheckSendCompatibility(vector<FairMQMessagePtr>& msgVec)
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

void FairMQChannel::CheckReceiveCompatibility(FairMQMessagePtr& msg)
{
    if (fTransportType != msg->GetType())
    {
        // LOG(debug) << "Channel type does not match message type. Creating wrapper";
        FairMQMessagePtr newMsg(NewMessage());
        msg = move(newMsg);
    }
}

void FairMQChannel::CheckReceiveCompatibility(vector<FairMQMessagePtr>& msgVec)
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
