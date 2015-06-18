/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQDevice.cxx
 *
 * @since 2012-10-25
 * @author D. Klein, A. Rybalchenko
 */

#include <list>
#include <algorithm> // for std::sort()

#include <boost/thread.hpp>
#include <boost/random/mersenne_twister.hpp> // for choosing random port in range
#include <boost/random/uniform_int_distribution.hpp> // for choosing random port in range

#include "FairMQSocket.h"
#include "FairMQDevice.h"
#include "FairMQLogger.h"

using namespace std;

FairMQDevice::FairMQDevice()
    : fChannels()
    , fId()
    , fMaxInitializationTime(120)
    , fNumIoThreads(1)
    , fPortRangeMin(22000)
    , fPortRangeMax(32000)
    , fLogIntervalInMs(1000)
    , fCommandSocket()
    , fTransportFactory(NULL)
{
}

void FairMQDevice::InitWrapper()
{
    LOG(INFO) << "DEVICE: Initializing...";

    fCommandSocket = fTransportFactory->CreateSocket("pair", "device-commands", 1);
    fCommandSocket->Bind("inproc://commands");

    // List to store the uninitialized channels.
    list<FairMQChannel*> uninitializedChannels;
    for (map< string,vector<FairMQChannel> >::iterator mi = fChannels.begin(); mi != fChannels.end(); ++mi)
    {
        for (vector<FairMQChannel>::iterator vi = (mi->second).begin(); vi != (mi->second).end(); ++vi)
        {
            // set channel name: name + vector index
            stringstream ss;
            ss << mi->first << "[" << vi - (mi->second).begin() << "]";
            vi->fChannelName = ss.str();
            // fill the uninitialized list
            uninitializedChannels.push_back(&(*vi));
        }
    }

    // go over the list of channels until all are initialized (and removed from the uninitialized list)
    int numAttempts = 0;
    int maxAttempts = fMaxInitializationTime;
    do {
        list<FairMQChannel*>::iterator itr = uninitializedChannels.begin();

        while (itr != uninitializedChannels.end())
        {
            if ((*itr)->ValidateChannel())
            {
                if (InitChannel(*(*itr)))
                {
                    uninitializedChannels.erase(itr++);
                }
                else
                {
                    LOG(ERROR) << "failed to initialize channel " << (*itr)->fChannelName;
                    ++itr;
                }
            }
            else
            {
                ++itr;
            }
        }

        ++numAttempts;
        if (numAttempts > maxAttempts)
        {
            LOG(ERROR) << "could not initialize all channels after " << maxAttempts << " attempts";
            // TODO: goto ERROR state;
            exit(EXIT_FAILURE);
        }

        if (numAttempts != 0)
        {
            boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
        }

    } while (!uninitializedChannels.empty());

    Init();

    ChangeState(internal_DEVICE_READY);

    // notify parent thread about end of processing.
    boost::lock_guard<boost::mutex> lock(fStateMutex);
    fStateFinished = true;
    fStateCondition.notify_one();
}

void FairMQDevice::Init()
{
}

bool FairMQDevice::InitChannel(FairMQChannel& ch)
{
    LOG(DEBUG) << "Initializing channel " << ch.fChannelName << " to <" << ch.fType << "> data";
    // initialize the socket
    ch.fSocket = fTransportFactory->CreateSocket(ch.fType, ch.fChannelName, 1);
    // set high water marks
    ch.fSocket->SetOption("snd-hwm", &(ch.fSndBufSize), sizeof(ch.fSndBufSize));
    ch.fSocket->SetOption("rcv-hwm", &(ch.fRcvBufSize), sizeof(ch.fRcvBufSize));

    // TODO: make it work with ipc

    if (ch.fMethod == "bind")
    {
        // number of attempts when choosing a random port
        int maxAttempts = 1000;
        int numAttempts = 0;

        // initialize random generator
        boost::random::mt19937 gen(getpid());
        boost::random::uniform_int_distribution<> randomPort(fPortRangeMin, fPortRangeMax);

        LOG(DEBUG) << "Binding channel " << ch.fChannelName << " on " << ch.fAddress;

        // try to bind to the saved port. In case of failure, try random one.
        if (!ch.fSocket->Bind(ch.fAddress))
        {
            LOG(DEBUG) << "Could not bind to configured port, trying random port in range " << fPortRangeMin << "-" << fPortRangeMax;
            do {
                ++numAttempts;

                if (numAttempts > maxAttempts)
                {
                    LOG(ERROR) << "could not bind to any port in the given range after " << maxAttempts << " attempts";
                    return false;
                }

                size_t pos = ch.fAddress.rfind(":");
                stringstream newPort;
                newPort << (int)randomPort(gen);
                ch.fAddress = ch.fAddress.substr(0, pos + 1) + newPort.str();

                LOG(DEBUG) << "Binding channel " << ch.fChannelName << " on " << ch.fAddress;
            } while (!ch.fSocket->Bind(ch.fAddress));
        }
    }
    else
    {
        LOG(DEBUG) << "Connecting channel " << ch.fChannelName << " to " << ch.fAddress;
        ch.fSocket->Connect(ch.fAddress);
    }

    return true;
}

void FairMQDevice::InitTaskWrapper()
{
    InitTask();

    ChangeState(internal_READY);

    // notify parent thread about end of processing.
    boost::lock_guard<boost::mutex> lock(fStateMutex);
    fStateFinished = true;
    fStateCondition.notify_one();
}

void FairMQDevice::InitTask()
{
}

bool FairMQDevice::SortSocketsByAddress(const FairMQChannel &lhs, const FairMQChannel &rhs)
{
    return lhs.fAddress < rhs.fAddress;
}

void FairMQDevice::SortChannel(const string& name, const bool reindex)
{
    if (fChannels.find(name) != fChannels.end())
    {
        sort(fChannels[name].begin(), fChannels[name].end(), SortSocketsByAddress);

        if (reindex)
        {
            for (vector<FairMQChannel>::iterator vi = fChannels[name].begin(); vi != fChannels[name].end(); ++vi)
            {
                // set channel name: name + vector index
                stringstream ss;
                ss << name << "[" << vi - fChannels[name].begin() << "]";
                vi->fChannelName = ss.str();
            }
        }
    }
    else
    {
        LOG(ERROR) << "Sorting failed: no channel with the name \"" << name << "\".";
    }
}

void FairMQDevice::PrintChannel(const string& name)
{
    if (fChannels.find(name) != fChannels.end())
    {
        for (vector<FairMQChannel>::iterator vi = fChannels[name].begin(); vi != fChannels[name].end(); ++vi)
        {
            LOG(INFO) << vi->fChannelName << ": "
                      << vi->fType << " | "
                      << vi->fMethod << " | "
                      << vi->fAddress << " | "
                      << vi->fSndBufSize << " | "
                      << vi->fRcvBufSize << " | "
                      << vi->fRateLogging;
        }
    }
    else
    {
        LOG(ERROR) << "Printing failed: no channel with the name \"" << name << "\".";
    }
}

void FairMQDevice::RunWrapper()
{
    boost::thread rateLogger(boost::bind(&FairMQDevice::LogSocketRates, this));

    Run();

    try {
        rateLogger.interrupt();
        rateLogger.join();
    } catch(boost::thread_resource_error& e) {
        LOG(ERROR) << e.what();
    }

    // notify parent thread about end of processing.
    boost::lock_guard<boost::mutex> lock(fStateMutex);
    fStateFinished = true;
    fStateCondition.notify_one();
}

void FairMQDevice::Run()
{
}

void FairMQDevice::Pause()
{
    while (GetCurrentState() == PAUSED)
    {
        try
        {
            boost::this_thread::sleep(boost::posix_time::milliseconds(500));
            LOG(DEBUG) << "paused...";
        }
        catch (boost::thread_interrupted&)
        {
            LOG(INFO) << "FairMQDevice::Pause() interrupted";
            break;
        }
    }
}

void FairMQDevice::ResetTaskWrapper()
{
    ResetTask();

    ChangeState(internal_DEVICE_READY);

    // notify parent thread about end of processing.
    boost::lock_guard<boost::mutex> lock(fStateMutex);
    fStateFinished = true;
    fStateCondition.notify_one();
}

void FairMQDevice::ResetTask()
{
}

void FairMQDevice::ResetWrapper()
{
    Reset();

    ChangeState(internal_IDLE);

    // notify parent thread about end of processing.
    boost::lock_guard<boost::mutex> lock(fStateMutex);
    fStateFinished = true;
    fStateCondition.notify_one();
}

void FairMQDevice::Reset()
{
}

// Method for setting properties represented as a string.
void FairMQDevice::SetProperty(const int key, const string& value)
{
    switch (key)
    {
        case Id:
            fId = value;
            break;
        default:
            FairMQConfigurable::SetProperty(key, value);
            break;
    }
}

// Method for setting properties represented as an integer.
void FairMQDevice::SetProperty(const int key, const int value)
{
    switch (key)
    {
        case NumIoThreads:
            fNumIoThreads = value;
            break;
        case MaxInitializationTime:
            fMaxInitializationTime = value;
            break;
        case PortRangeMin:
            fPortRangeMin = value;
            break;
        case PortRangeMax:
            fPortRangeMax = value;
            break;
        case LogIntervalInMs:
            fLogIntervalInMs = value;
            break;
        default:
            FairMQConfigurable::SetProperty(key, value);
            break;
    }
}

// Method for getting properties represented as an string.
string FairMQDevice::GetProperty(const int key, const string& default_ /*= ""*/)
{
    switch (key)
    {
        case Id:
            return fId;
        default:
            return FairMQConfigurable::GetProperty(key, default_);
    }
}

// Method for getting properties represented as an integer.
int FairMQDevice::GetProperty(const int key, const int default_ /*= 0*/)
{
    switch (key)
    {
        case NumIoThreads:
            return fNumIoThreads;
        case MaxInitializationTime:
            return fMaxInitializationTime;
        case PortRangeMin:
            return fPortRangeMin;
        case PortRangeMax:
            return fPortRangeMax;
        case LogIntervalInMs:
            return fLogIntervalInMs;
        default:
            return FairMQConfigurable::GetProperty(key, default_);
    }
}

void FairMQDevice::SetTransport(FairMQTransportFactory* factory)
{
    fTransportFactory = factory;
}

void FairMQDevice::LogSocketRates()
{
    timestamp_t t0;
    timestamp_t t1;

    timestamp_t msSinceLastLog;

    int numFilteredSockets = 0;
    vector<FairMQSocket*> filteredSockets;
    vector<string> filteredChannelNames;

    // iterate over the channels map
    for (map< string,vector<FairMQChannel> >::iterator mi = fChannels.begin(); mi != fChannels.end(); ++mi)
    {
        // iterate over the channels vector
        for (vector<FairMQChannel>::iterator vi = (mi->second).begin(); vi != (mi->second).end(); ++vi)
        {
            if (vi->fRateLogging == 1)
            {
                filteredSockets.push_back(vi->fSocket);
                stringstream ss;
                ss << mi->first << "[" << vi - (mi->second).begin() << "]";
                filteredChannelNames.push_back(ss.str());
                ++numFilteredSockets;
            }
        }
    }

    vector<unsigned long> bytesIn(numFilteredSockets);
    vector<unsigned long> msgIn(numFilteredSockets);
    vector<unsigned long> bytesOut(numFilteredSockets);
    vector<unsigned long> msgOut(numFilteredSockets);

    vector<unsigned long> bytesInNew(numFilteredSockets);
    vector<unsigned long> msgInNew(numFilteredSockets);
    vector<unsigned long> bytesOutNew(numFilteredSockets);
    vector<unsigned long> msgOutNew(numFilteredSockets);

    vector<double> mbPerSecIn(numFilteredSockets);
    vector<double> msgPerSecIn(numFilteredSockets);
    vector<double> mbPerSecOut(numFilteredSockets);
    vector<double> msgPerSecOut(numFilteredSockets);

    int i = 0;
    for (vector<FairMQSocket*>::iterator itr = filteredSockets.begin(); itr != filteredSockets.end(); ++itr)
    {
        bytesIn.at(i) = (*itr)->GetBytesRx();
        bytesOut.at(i) = (*itr)->GetBytesTx();
        msgIn.at(i) = (*itr)->GetMessagesRx();
        msgOut.at(i) = (*itr)->GetMessagesTx();
        ++i;
    }

    t0 = get_timestamp();

    while (GetCurrentState() == RUNNING)
    {
        try
        {
            t1 = get_timestamp();

            msSinceLastLog = (t1 - t0) / 1000.0L;

            i = 0;

            for (vector<FairMQSocket*>::iterator itr = filteredSockets.begin(); itr != filteredSockets.end(); itr++)
            {
                bytesInNew.at(i) = (*itr)->GetBytesRx();
                mbPerSecIn.at(i) = ((double)(bytesInNew.at(i) - bytesIn.at(i)) / (1024. * 1024.)) / (double)msSinceLastLog * 1000.;
                bytesIn.at(i) = bytesInNew.at(i);

                msgInNew.at(i) = (*itr)->GetMessagesRx();
                msgPerSecIn.at(i) = (double)(msgInNew.at(i) - msgIn.at(i)) / (double)msSinceLastLog * 1000.;
                msgIn.at(i) = msgInNew.at(i);

                bytesOutNew.at(i) = (*itr)->GetBytesTx();
                mbPerSecOut.at(i) = ((double)(bytesOutNew.at(i) - bytesOut.at(i)) / (1024. * 1024.)) / (double)msSinceLastLog * 1000.;
                bytesOut.at(i) = bytesOutNew.at(i);

                msgOutNew.at(i) = (*itr)->GetMessagesTx();
                msgPerSecOut.at(i) = (double)(msgOutNew.at(i) - msgOut.at(i)) / (double)msSinceLastLog * 1000.;
                msgOut.at(i) = msgOutNew.at(i);

                LOG(DEBUG) << filteredChannelNames.at(i) << ": "
                           << "in: " << msgPerSecIn.at(i) << " msg (" << mbPerSecIn.at(i) << " MB), "
                           << "out: " << msgPerSecOut.at(i) << " msg (" << mbPerSecOut.at(i) << " MB)";

                ++i;
            }

            t0 = t1;
            boost::this_thread::sleep(boost::posix_time::milliseconds(fLogIntervalInMs));
        }
        catch (boost::thread_interrupted&)
        {
            // LOG(DEBUG) << "FairMQDevice::LogSocketRates() interrupted";
            break;
        }
    }

    // LOG(DEBUG) << "FairMQDevice::LogSocketRates() stopping";
}

void FairMQDevice::SendCommand(const string& command)
{
    FairMQMessage* cmd = fTransportFactory->CreateMessage(command.size());
    memcpy(cmd->GetData(), command.c_str(), command.size());
    fCommandSocket->Send(cmd, 0);
}

void FairMQDevice::Shutdown()
{
    LOG(DEBUG) << "Closing sockets...";

    // iterate over the channels map
    for (map< string,vector<FairMQChannel> >::iterator mi = fChannels.begin(); mi != fChannels.end(); ++mi)
    {
        // iterate over the channels vector
        for (vector<FairMQChannel>::iterator vi = (mi->second).begin(); vi != (mi->second).end(); ++vi)
        {
            vi->fSocket->Close();
        }
    }

    fCommandSocket->Close();

    LOG(DEBUG) << "Closed all sockets!";
}

void FairMQDevice::Terminate()
{
    // Termination signal has to be sent only once to any socket.
    fCommandSocket->Terminate();
}

FairMQDevice::~FairMQDevice()
{
    // iterate over the channels map
    for (map< string,vector<FairMQChannel> >::iterator mi = fChannels.begin(); mi != fChannels.end(); ++mi)
    {
        // iterate over the channels vector
        for (vector<FairMQChannel>::iterator vi = (mi->second).begin(); vi != (mi->second).end(); ++vi)
        {
            delete vi->fSocket;
        }
    }

    delete fCommandSocket;
}
