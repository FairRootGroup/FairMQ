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
#include <utility> // std::move

#include <boost/algorithm/string.hpp> // join/split

#include "FairMQChannel.h"

using namespace std;

mutex FairMQChannel::fChannelMutex;

atomic<bool> FairMQChannel::fInterrupted(false);

FairMQChannel::FairMQChannel()
    : fSocket(nullptr)
    , fType("unspecified")
    , fMethod("unspecified")
    , fAddress("unspecified")
    , fTransport("default")
    , fSndBufSize(1000)
    , fRcvBufSize(1000)
    , fSndKernelSize(0)
    , fRcvKernelSize(0)
    , fRateLogging(1)
    , fName("")
    , fIsValid(false)
    , fPoller(nullptr)
    , fChannelCmdSocket(nullptr)
    , fTransportType(FairMQ::Transport::DEFAULT)
    , fTransportFactory(nullptr)
    , fNoBlockFlag(0)
    , fSndMoreFlag(0)
    , fMultipart(false)
{
}

FairMQChannel::FairMQChannel(const string& type, const string& method, const string& address)
    : fSocket(nullptr)
    , fType(type)
    , fMethod(method)
    , fAddress(address)
    , fTransport("default")
    , fSndBufSize(1000)
    , fRcvBufSize(1000)
    , fSndKernelSize(0)
    , fRcvKernelSize(0)
    , fRateLogging(1)
    , fName("")
    , fIsValid(false)
    , fPoller(nullptr)
    , fChannelCmdSocket(nullptr)
    , fTransportType(FairMQ::Transport::DEFAULT)
    , fTransportFactory(nullptr)
    , fNoBlockFlag(0)
    , fSndMoreFlag(0)
    , fMultipart(false)
{
}

FairMQChannel::FairMQChannel(const FairMQChannel& chan)
    : fSocket(nullptr)
    , fType(chan.fType)
    , fMethod(chan.fMethod)
    , fAddress(chan.fAddress)
    , fTransport(chan.fTransport)
    , fSndBufSize(chan.fSndBufSize)
    , fRcvBufSize(chan.fRcvBufSize)
    , fSndKernelSize(chan.fSndKernelSize)
    , fRcvKernelSize(chan.fRcvKernelSize)
    , fRateLogging(chan.fRateLogging)
    , fName(chan.fName)
    , fIsValid(false)
    , fPoller(nullptr)
    , fChannelCmdSocket(nullptr)
    , fTransportType(FairMQ::Transport::DEFAULT)
    , fTransportFactory(nullptr)
    , fNoBlockFlag(chan.fNoBlockFlag)
    , fSndMoreFlag(chan.fSndMoreFlag)
    , fMultipart(chan.fMultipart)
{}

FairMQChannel& FairMQChannel::operator=(const FairMQChannel& chan)
{
    fType = chan.fType;
    fMethod = chan.fMethod;
    fAddress = chan.fAddress;
    fTransport = chan.fTransport;
    fSndBufSize = chan.fSndBufSize;
    fRcvBufSize = chan.fRcvBufSize;
    fSndKernelSize = chan.fSndKernelSize;
    fRcvKernelSize = chan.fRcvKernelSize;
    fRateLogging = chan.fRateLogging;
    fSocket = nullptr;
    fName = chan.fName;
    fIsValid = false;
    fPoller = nullptr;
    fChannelCmdSocket = nullptr;
    fTransportType = FairMQ::Transport::DEFAULT;
    fTransportFactory = nullptr;
    fNoBlockFlag = chan.fNoBlockFlag;
    fSndMoreFlag = chan.fSndMoreFlag;

    return *this;
}

FairMQSocket const & FairMQChannel::GetSocket() const
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
    return prefix.erase(fName.rfind("["));
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
        LOG(ERROR) << "Exception caught in FairMQChannel::GetType: " << e.what();
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
        LOG(ERROR) << "Exception caught in FairMQChannel::GetMethod: " << e.what();
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
        LOG(ERROR) << "Exception caught in FairMQChannel::GetAddress: " << e.what();
        exit(EXIT_FAILURE);
    }
}

string FairMQChannel::GetTransport() const
{
    try
    {
        unique_lock<mutex> lock(fChannelMutex);
        return fTransport;
    }
    catch (exception& e)
    {
        LOG(ERROR) << "Exception caught in FairMQChannel::GetTransport: " << e.what();
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
        LOG(ERROR) << "Exception caught in FairMQChannel::GetSndBufSize: " << e.what();
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
        LOG(ERROR) << "Exception caught in FairMQChannel::GetRcvBufSize: " << e.what();
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
        LOG(ERROR) << "Exception caught in FairMQChannel::GetSndKernelSize: " << e.what();
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
        LOG(ERROR) << "Exception caught in FairMQChannel::GetRcvKernelSize: " << e.what();
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
        LOG(ERROR) << "Exception caught in FairMQChannel::GetRateLogging: " << e.what();
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
    }
    catch (exception& e)
    {
        LOG(ERROR) << "Exception caught in FairMQChannel::UpdateType: " << e.what();
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
    }
    catch (exception& e)
    {
        LOG(ERROR) << "Exception caught in FairMQChannel::UpdateMethod: " << e.what();
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
    }
    catch (exception& e)
    {
        LOG(ERROR) << "Exception caught in FairMQChannel::UpdateAddress: " << e.what();
        exit(EXIT_FAILURE);
    }
}

void FairMQChannel::UpdateTransport(const string& transport)
{
    try
    {
        unique_lock<mutex> lock(fChannelMutex);
        fIsValid = false;
        fTransport = transport;
    }
    catch (exception& e)
    {
        LOG(ERROR) << "Exception caught in FairMQChannel::UpdateTransport: " << e.what();
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
    }
    catch (exception& e)
    {
        LOG(ERROR) << "Exception caught in FairMQChannel::UpdateSndBufSize: " << e.what();
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
    }
    catch (exception& e)
    {
        LOG(ERROR) << "Exception caught in FairMQChannel::UpdateRcvBufSize: " << e.what();
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
    }
    catch (exception& e)
    {
        LOG(ERROR) << "Exception caught in FairMQChannel::UpdateSndKernelSize: " << e.what();
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
    }
    catch (exception& e)
    {
        LOG(ERROR) << "Exception caught in FairMQChannel::UpdateRcvKernelSize: " << e.what();
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
    }
    catch (exception& e)
    {
        LOG(ERROR) << "Exception caught in FairMQChannel::UpdateRateLogging: " << e.what();
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
        LOG(ERROR) << "Exception caught in FairMQChannel::IsValid: " << e.what();
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
            LOG(DEBUG) << ss.str();
            return true;
        }

        // validate socket type
        const string socketTypeNames[] = { "sub", "pub", "pull", "push", "req", "rep", "xsub", "xpub", "dealer", "router", "pair" };
        const set<string> socketTypes(socketTypeNames, socketTypeNames + sizeof(socketTypeNames) / sizeof(string));
        if (socketTypes.find(fType) == socketTypes.end())
        {
            ss << "INVALID";
            LOG(DEBUG) << ss.str();
            LOG(ERROR) << "Invalid channel type: \"" << fType << "\"";
            exit(EXIT_FAILURE);
        }

        // validate socket address
        if (fAddress == "unspecified" || fAddress == "")
        {
            ss << "INVALID";
            LOG(DEBUG) << ss.str();
            LOG(DEBUG) << "invalid channel address: \"" << fAddress << "\"";
            return false;
        }
        else
        {
            //TODO: maybe cache fEndpoints as a class member? not really needed as tokenizing is
            //fast, and only happens during (re-)configure
            vector<string> fEndpoints;
            Tokenize(fEndpoints, fAddress);
            for (const auto endpoint : fEndpoints)
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
                        LOG(DEBUG) << ss.str();
                        LOG(ERROR) << "Invalid endpoint connection method: \"" << fMethod << "\" for " << endpoint;
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
                        LOG(DEBUG) << ss.str();
                        LOG(ERROR) << "invalid channel address: \"" << address << "\" (missing port?)";
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
                        LOG(DEBUG) << ss.str();
                        LOG(ERROR) << "invalid channel address: \"" << address << "\" (empty IPC address?)";
                        return false;
                    }
                }
                else
                {
                    // if neither TCP or IPC is specified, return invalid
                    ss << "INVALID";
                    LOG(DEBUG) << ss.str();
                    LOG(ERROR) << "invalid channel address: \"" << address << "\" (missing protocol specifier?)";
                    return false;
                }
            }
        }

        // validate channel transport
        // const string channelTransportNames[] = { "default", "zeromq", "nanomsg", "shmem" };
        // const set<string> channelTransports(channelTransportNames, channelTransportNames + sizeof(channelTransportNames) / sizeof(string));
        if (FairMQ::TransportTypes.find(fTransport) == FairMQ::TransportTypes.end())
        {
            ss << "INVALID";
            LOG(DEBUG) << ss.str();
            LOG(ERROR) << "Invalid channel transport: \"" << fTransport << "\"";
            exit(EXIT_FAILURE);
        }

        // validate socket buffer size for sending
        if (fSndBufSize < 0)
        {
            ss << "INVALID";
            LOG(DEBUG) << ss.str();
            LOG(ERROR) << "invalid channel send buffer size (cannot be negative): \"" << fSndBufSize << "\"";
            exit(EXIT_FAILURE);
        }

        // validate socket buffer size for receiving
        if (fRcvBufSize < 0)
        {
            ss << "INVALID";
            LOG(DEBUG) << ss.str();
            LOG(ERROR) << "invalid channel receive buffer size (cannot be negative): \"" << fRcvBufSize << "\"";
            exit(EXIT_FAILURE);
        }

        // validate socket kernel transmit size for sending
        if (fSndKernelSize < 0)
        {
            ss << "INVALID";
            LOG(DEBUG) << ss.str();
            LOG(ERROR) << "invalid channel send kernel transmit size (cannot be negative): \"" << fSndKernelSize << "\"";
            exit(EXIT_FAILURE);
        }

        // validate socket kernel transmit size for receiving
        if (fRcvKernelSize < 0)
        {
            ss << "INVALID";
            LOG(DEBUG) << ss.str();
            LOG(ERROR) << "invalid channel receive kernel transmit size (cannot be negative): \"" << fRcvKernelSize << "\"";
            exit(EXIT_FAILURE);
        }

        // validate socket rate logging interval
        if (fRateLogging < 0)
        {
            ss << "INVALID";
            LOG(DEBUG) << ss.str();
            LOG(ERROR) << "invalid socket rate logging interval (cannot be negative): \"" << fRateLogging << "\"";
            exit(EXIT_FAILURE);
        }

        fIsValid = true;
        ss << "VALID";
        LOG(DEBUG) << ss.str();
        return true;
    }
    catch (exception& e)
    {
        LOG(ERROR) << "Exception caught in FairMQChannel::ValidateChannel: " << e.what();
        exit(EXIT_FAILURE);
    }
}

void FairMQChannel::InitTransport(shared_ptr<FairMQTransportFactory> factory)
{
    fTransportFactory = factory;
    fTransportType = factory->GetType();
}

bool FairMQChannel::InitCommandInterface()
{
    fChannelCmdSocket = fTransportFactory->CreateSocket("sub", "device-commands", "internal");
    if (fChannelCmdSocket)
    {
        fChannelCmdSocket->Connect("inproc://commands");

        fNoBlockFlag = fChannelCmdSocket->NOBLOCK;
        fSndMoreFlag = fChannelCmdSocket->SNDMORE;

        fPoller = fTransportFactory->CreatePoller(*fChannelCmdSocket, *fSocket);

        return true;
    }
    else
    {
        return false;
    }
}

void FairMQChannel::ResetChannel()
{
    unique_lock<mutex> lock(fChannelMutex);
    fIsValid = false;
    // TODO: implement channel resetting
}

int FairMQChannel::Send(unique_ptr<FairMQMessage>& msg) const
{
    CheckCompatibility(msg);
    return fSocket->Send(msg);
}

int FairMQChannel::Receive(unique_ptr<FairMQMessage>& msg) const
{
    CheckCompatibility(msg);
    return fSocket->Receive(msg);
}

int FairMQChannel::Send(unique_ptr<FairMQMessage>& msg, int sndTimeoutInMs) const
{
    fPoller->Poll(sndTimeoutInMs);

    if (fPoller->CheckInput(0))
    {
        HandleUnblock();
        if (fInterrupted)
        {
            return -2;
        }
    }

    if (fPoller->CheckOutput(1))
    {
        return Send(msg);
    }

    return -2;
}

int FairMQChannel::Receive(unique_ptr<FairMQMessage>& msg, int rcvTimeoutInMs) const
{
    fPoller->Poll(rcvTimeoutInMs);

    if (fPoller->CheckInput(0))
    {
        HandleUnblock();
        if (fInterrupted)
        {
            return -2;
        }
    }

    if (fPoller->CheckInput(1))
    {
        return Receive(msg);
    }

    return -2;
}

int FairMQChannel::SendAsync(unique_ptr<FairMQMessage>& msg) const
{
    CheckCompatibility(msg);
    return fSocket->Send(msg, fNoBlockFlag);
}

int FairMQChannel::ReceiveAsync(unique_ptr<FairMQMessage>& msg) const
{
    CheckCompatibility(msg);
    return fSocket->Receive(msg, fNoBlockFlag);
}

int64_t FairMQChannel::Send(vector<unique_ptr<FairMQMessage>>& msgVec) const
{
    CheckCompatibility(msgVec);
    return fSocket->Send(msgVec);
}

int64_t FairMQChannel::Receive(vector<unique_ptr<FairMQMessage>>& msgVec) const
{
    CheckCompatibility(msgVec);
    return fSocket->Receive(msgVec);
}

int64_t FairMQChannel::Send(vector<unique_ptr<FairMQMessage>>& msgVec, int sndTimeoutInMs) const
{
    fPoller->Poll(sndTimeoutInMs);

    if (fPoller->CheckInput(0))
    {
        HandleUnblock();
        if (fInterrupted)
        {
            return -2;
        }
    }

    if (fPoller->CheckOutput(1))
    {
        return Send(msgVec);
    }

    return -2;
}

int64_t FairMQChannel::Receive(vector<unique_ptr<FairMQMessage>>& msgVec, int rcvTimeoutInMs) const
{
    fPoller->Poll(rcvTimeoutInMs);

    if (fPoller->CheckInput(0))
    {
        HandleUnblock();
        if (fInterrupted)
        {
            return -2;
        }
    }

    if (fPoller->CheckInput(1))
    {
        return Receive(msgVec);
    }

    return -2;
}

int64_t FairMQChannel::SendAsync(vector<unique_ptr<FairMQMessage>>& msgVec) const
{
    CheckCompatibility(msgVec);
    return fSocket->Send(msgVec, fNoBlockFlag);
}

/// Receives a vector of messages in non-blocking mode.
///
/// @param msgVec message vector reference
/// @return Number of bytes that have been received. If queue is empty, returns -2.
/// In case of errors, returns -1.
int64_t FairMQChannel::ReceiveAsync(vector<unique_ptr<FairMQMessage>>& msgVec) const
{
    CheckCompatibility(msgVec);
    return fSocket->Receive(msgVec, fNoBlockFlag);
}

inline bool FairMQChannel::HandleUnblock() const
{
    FairMQMessagePtr cmd(fTransportFactory->CreateMessage());
    if (fChannelCmdSocket->Receive(cmd) >= 0)
    {
        // LOG(DEBUG) << "unblocked";
    }
    return true;
}

FairMQChannel::~FairMQChannel()
{
}

void FairMQChannel::Tokenize(vector<string>& output, const string& input, const string delimiters)
{
    boost::algorithm::split(output, input, boost::algorithm::is_any_of(delimiters));
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


FairMQTransportFactory* FairMQChannel::Transport()
{
    return fTransportFactory.get();
}

bool FairMQChannel::CheckCompatibility(unique_ptr<FairMQMessage>& msg) const
{
    if (fTransportType == msg->GetType())
    {
        return true;
    }
    else
    {
        // LOG(WARN) << "Channel type does not match message type. Copying...";
        FairMQMessagePtr msgCopy(fTransportFactory->CreateMessage(msg->GetSize()));
        memcpy(msgCopy->GetData(), msg->GetData(), msg->GetSize());
        msg = move(msgCopy);
        return false;
    }
}

bool FairMQChannel::CheckCompatibility(vector<unique_ptr<FairMQMessage>>& msgVec) const
{
    if (msgVec.size() > 0)
    {
        if (fTransportType == msgVec.at(0)->GetType())
        {
            return true;
        }
        else
        {
            // LOG(WARN) << "Channel type does not match message type. Copying...";
            vector<unique_ptr<FairMQMessage>> tempVec;
            for (unsigned int i = 0; i < msgVec.size(); ++i)
            {
                tempVec.push_back(fTransportFactory->CreateMessage(msgVec[i]->GetSize()));
                memcpy(tempVec[i]->GetData(), msgVec[i]->GetData(), msgVec[i]->GetSize());
            }
            msgVec = move(tempVec);
            return false;
        }
    }
    else
    {
        return true;
    }
}
