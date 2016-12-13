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
#include <algorithm> // std::sort()
#include <csignal> // catching system signals
#include <cstdlib>
#include <stdexcept>
#include <random>
#include <chrono>
#include <mutex>
#include <thread>
#include <functional>

#include <termios.h> // for the InteractiveStateLoop
#include <poll.h>

#include <boost/algorithm/string.hpp> // join/split

#include "FairMQSocket.h"
#include "FairMQDevice.h"
#include "FairMQLogger.h"
#include "FairMQTools.h"

#include "FairMQProgOptions.h"
#include "FairMQTransportFactoryZMQ.h"
#ifdef NANOMSG_FOUND
#include "FairMQTransportFactoryNN.h"
#endif

using namespace std;

// std::function and a wrapper to catch the signals
std::function<void(int)> sigHandler;
static void CallSignalHandler(int signal)
{
    sigHandler(signal);
}

FairMQDevice::FairMQDevice()
    : fChannels()
    , fConfig(nullptr)
    , fId()
    , fNetworkInterface()
    , fMaxInitializationAttempts(120)
    , fNumIoThreads(1)
    , fPortRangeMin(22000)
    , fPortRangeMax(32000)
    , fLogIntervalInMs(1000)
    , fCmdSocket(nullptr)
    , fTransportFactory(nullptr)
    , fInitialValidationFinished(false)
    , fInitialValidationCondition()
    , fInitialValidationMutex()
    , fCatchingSignals(false)
    , fTerminationRequested(false)
    , fInteractiveRunning(false)
    , fDataCallbacks(false)
    , fMsgInputs()
    , fMultipartInputs()
{
}

void FairMQDevice::CatchSignals()
{
    if (!fCatchingSignals)
    {
        sigHandler = bind1st(mem_fun(&FairMQDevice::SignalHandler), this);
        std::signal(SIGINT, CallSignalHandler);
        std::signal(SIGTERM, CallSignalHandler);
        fCatchingSignals = true;
    }
}

void FairMQDevice::SignalHandler(int signal)
{
    LOG(INFO) << "Caught signal " << signal;

    if (!fTerminationRequested)
    {
        fTerminationRequested = true;

        ChangeState(STOP);

        ChangeState(RESET_TASK);
        WaitForEndOfState(RESET_TASK);

        ChangeState(RESET_DEVICE);
        WaitForEndOfState(RESET_DEVICE);

        ChangeState(END);

        // exit(EXIT_FAILURE);
        fInteractiveRunning = false;
        LOG(INFO) << "Exiting.";
    }
    else
    {
        LOG(WARN) << "Repeated termination or bad initialization? Aborting.";
        std::abort();
        // exit(EXIT_FAILURE);
    }
}

void FairMQDevice::ConnectChannels(list<FairMQChannel*>& chans)
{
    auto itr = chans.begin();

    while (itr != chans.end())
    {
        if ((*itr)->ValidateChannel())
        {
            if (AttachChannel(**itr))
            {
                (*itr)->InitCommandInterface(fTransportFactory, fNumIoThreads);
                chans.erase(itr++);
            }
            else
            {
                LOG(ERROR) << "failed to connect channel " << (*itr)->fChannelName;
                ++itr;
            }
        }
        else
        {
            ++itr;
        }
    }
}

void FairMQDevice::BindChannels(list<FairMQChannel*>& chans)
{
    auto itr = chans.begin();

    while (itr != chans.end())
    {
        if ((*itr)->ValidateChannel())
        {
            if (AttachChannel(**itr))
            {
                (*itr)->InitCommandInterface(fTransportFactory, fNumIoThreads);
                chans.erase(itr++);
            }
            else
            {
                LOG(ERROR) << "failed to bind channel " << (*itr)->fChannelName;
                ++itr;
            }
        }
        else
        {
            ++itr;
        }
    }
}

void FairMQDevice::InitWrapper()
{
    if (!fTransportFactory)
    {
        LOG(ERROR) << "Transport not initialized. Did you call SetTransport()?";
        exit(EXIT_FAILURE);
    }

    if (!fCmdSocket)
    {
        fCmdSocket = fTransportFactory->CreateSocket("pub", "device-commands", fNumIoThreads, fId);
        fCmdSocket->Bind("inproc://commands");
    }

    // Containers to store the uninitialized channels.
    list<FairMQChannel*> uninitializedBindingChannels;
    list<FairMQChannel*> uninitializedConnectingChannels;

    // Fill the uninitialized channel containers
    for (auto mi = fChannels.begin(); mi != fChannels.end(); ++mi)
    {
        for (auto vi = (mi->second).begin(); vi != (mi->second).end(); ++vi)
        {
            if (vi->fMethod == "bind")
            {
                // set channel name: name + vector index
                stringstream ss;
                ss << mi->first << "[" << vi - (mi->second).begin() << "]";
                vi->fChannelName = ss.str();
                // if binding address is not specified, set it up to try getting it from the configured network interface
                if (vi->fAddress == "unspecified" || vi->fAddress == "")
                {
                    vi->fAddress = "tcp://" + FairMQ::tools::getInterfaceIP(fNetworkInterface) + ":1";
                }
                // fill the uninitialized list
                uninitializedBindingChannels.push_back(&(*vi));
            }
            else if (vi->fMethod == "connect")
            {
                // set channel name: name + vector index
                stringstream ss;
                ss << mi->first << "[" << vi - (mi->second).begin() << "]";
                vi->fChannelName = ss.str();
                // fill the uninitialized list
                uninitializedConnectingChannels.push_back(&(*vi));
            }
            else if (vi->fAddress.find_first_of("@+>") != string::npos)
            {
                // set channel name: name + vector index
                stringstream ss;
                ss << mi->first << "[" << vi - (mi->second).begin() << "]";
                vi->fChannelName = ss.str();
                // fill the uninitialized list
                uninitializedConnectingChannels.push_back(&(*vi));
            }
            else
            {
                LOG(ERROR) << "Cannot update configuration. Socket method (bind/connect) not specified.";
                exit(EXIT_FAILURE);
            }
        }
    }

    // Bind channels. Here one run is enough, because bind settings should be available locally
    // If necessary this could be handled in the same way as the connecting channels
    BindChannels(uninitializedBindingChannels);
    // notify parent thread about completion of first validation.
    {
        lock_guard<mutex> lock(fInitialValidationMutex);
        fInitialValidationFinished = true;
        fInitialValidationCondition.notify_one();
    }

    // go over the list of channels until all are initialized (and removed from the uninitialized list)
    int numAttempts = 0;
    while (!uninitializedConnectingChannels.empty())
    {
        ConnectChannels(uninitializedConnectingChannels);
        if (++numAttempts > fMaxInitializationAttempts)
        {
            LOG(ERROR) << "could not connect all channels after " << fMaxInitializationAttempts << " attempts";
            // TODO: goto ERROR state;
            exit(EXIT_FAILURE);
        }

        if (numAttempts != 0)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    }

    Init();

    ChangeState(internal_DEVICE_READY);
}

void FairMQDevice::WaitForInitialValidation()
{
    unique_lock<mutex> lock(fInitialValidationMutex);
    fInitialValidationCondition.wait(lock, [&] () { return fInitialValidationFinished; });
}

void FairMQDevice::Init()
{
}

bool FairMQDevice::BindChannel(FairMQChannel& ch)
{
    LOG(DEBUG) << "Initializing channel " << ch.fChannelName << " (" << ch.fType << ")";
    // initialize the socket
    ch.fSocket = fTransportFactory->CreateSocket(ch.fType, ch.fChannelName, fNumIoThreads, fId);
    // set high water marks
    ch.fSocket->SetOption("snd-hwm", &(ch.fSndBufSize), sizeof(ch.fSndBufSize));
    ch.fSocket->SetOption("rcv-hwm", &(ch.fRcvBufSize), sizeof(ch.fRcvBufSize));

    LOG(DEBUG) << "Binding channel " << ch.fChannelName << " on " << ch.fAddress;

    return BindEndpoint(*ch.fSocket, ch.fAddress);
}

bool FairMQDevice::ConnectChannel(FairMQChannel& ch)
{
    LOG(DEBUG) << "Initializing channel " << ch.fChannelName << " (" << ch.fType << ")";
    // initialize the socket
    ch.fSocket = fTransportFactory->CreateSocket(ch.fType, ch.fChannelName, fNumIoThreads, fId);
    // set high water marks
    ch.fSocket->SetOption("snd-hwm", &(ch.fSndBufSize), sizeof(ch.fSndBufSize));
    ch.fSocket->SetOption("rcv-hwm", &(ch.fRcvBufSize), sizeof(ch.fRcvBufSize));
    // connect
    LOG(DEBUG) << "Connecting channel " << ch.fChannelName << " to " << ch.fAddress;
    ConnectEndpoint(*ch.fSocket, ch.fAddress);
    return true;
}

bool FairMQDevice::AttachChannel(FairMQChannel& ch)
{
  std::vector<std::string> endpoints;
  FairMQChannel::Tokenize(endpoints, ch.fAddress);
  for (auto& endpoint : endpoints)
  {
    //(re-)init socket
    if (!ch.fSocket) {
      ch.fSocket = fTransportFactory->CreateSocket(ch.fType, ch.fChannelName, fNumIoThreads, fId);
    }

    // set high water marks
    ch.fSocket->SetOption("snd-hwm", &(ch.fSndBufSize), sizeof(ch.fSndBufSize));
    ch.fSocket->SetOption("rcv-hwm", &(ch.fRcvBufSize), sizeof(ch.fRcvBufSize));

    // attach
    bool bind = (ch.fMethod=="bind");
    bool connectionModifier = false;
    std::string address = endpoint;

    // check if the default fMethod is overridden by a modifier
    if (endpoint[0]=='+' || endpoint[0]=='>') {
      connectionModifier = true;
      bind = false;
      address = endpoint.substr(1);
    } else if (endpoint[0]=='@') {
      connectionModifier = true;
      bind = true;
      address = endpoint.substr(1);
    }

    bool rc = true;
    // make the connection
    if (bind) {
      rc = BindEndpoint(*ch.fSocket, address);
    } else {
      rc = ConnectEndpoint(*ch.fSocket, address);
    }

    // bind might bind to an address different than requested,
    // put the actual address back in the config
    endpoint.clear();
    if (connectionModifier) endpoint.push_back(bind?'@':'+');
    endpoint += address;

    LOG(DEBUG) << "Attached channel " << ch.fChannelName << " to " << endpoint
      << (bind?" (bind) ":" (connect) ");

    // after the book keeping is done, exit in case of errors
    if (!rc) return rc;
  }

  // put the (possibly) modified address back in the config
  ch.UpdateAddress(boost::algorithm::join(endpoints, ","));

  return true;
}

bool FairMQDevice::ConnectEndpoint(FairMQSocket& socket, std::string& endpoint)
{
  socket.Connect(endpoint);
  return true;
}

bool FairMQDevice::BindEndpoint(FairMQSocket& socket, std::string& endpoint)
{
    // number of attempts when choosing a random port
    int maxAttempts = 1000;
    int numAttempts = 0;

    // initialize random generator
    std::default_random_engine generator(std::chrono::system_clock::now().time_since_epoch().count());
    std::uniform_int_distribution<int> randomPort(fPortRangeMin, fPortRangeMax);

    // try to bind to the saved port. In case of failure, try random one.
    while (!socket.Bind(endpoint))
    {
        LOG(DEBUG) << "Could not bind to configured (TCP) port, trying random port in range "
          << fPortRangeMin << "-" << fPortRangeMax;
        ++numAttempts;

        if (numAttempts > maxAttempts)
        {
            LOG(ERROR) << "could not bind to any (TCP) port in the given range after "
              << maxAttempts << " attempts";
            return false;
        }

        size_t pos = endpoint.rfind(":");
        stringstream newPort;
        newPort << static_cast<int>(randomPort(generator));
        // TODO: thread safety? (this comes in as a reference and DOES get changed in this case).
        endpoint = endpoint.substr(0, pos + 1) + newPort.str();
    }
    return true;
}

void FairMQDevice::InitTaskWrapper()
{
    InitTask();

    ChangeState(internal_READY);
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
        sort(fChannels.at(name).begin(), fChannels.at(name).end(), SortSocketsByAddress);

        if (reindex)
        {
            for (auto vi = fChannels.at(name).begin(); vi != fChannels.at(name).end(); ++vi)
            {
                // set channel name: name + vector index
                stringstream ss;
                ss << name << "[" << vi - fChannels.at(name).begin() << "]";
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
        for (auto vi = fChannels[name].begin(); vi != fChannels[name].end(); ++vi)
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

void FairMQDevice::OnData(const string& channelName, InputMsgCallback callback)
{
    fDataCallbacks = true;
    fMsgInputs.insert(make_pair(channelName, callback));
}

void FairMQDevice::OnData(const string& channelName, InputMultipartCallback callback)
{
    fDataCallbacks = true;
    fMultipartInputs.insert(make_pair(channelName, callback));
}

void FairMQDevice::RunWrapper()
{
    LOG(INFO) << "DEVICE: Running...";

    std::thread rateLogger(&FairMQDevice::LogSocketRates, this);

    FairMQChannel::fInterrupted = false;

    try
    {
        PreRun();

        if (fDataCallbacks)
        {
            bool exitingRunningCallback = false;

            vector<string> inputChannelKeys;
            for (const auto& i: fMsgInputs)
            {
                inputChannelKeys.push_back(i.first);
            }
            for (const auto& i: fMultipartInputs)
            {
                inputChannelKeys.push_back(i.first);
            }

            FairMQPollerPtr poller(fTransportFactory->CreatePoller(fChannels, inputChannelKeys));

            while (CheckCurrentState(RUNNING) && !exitingRunningCallback)
            {
                poller->Poll(200);

                for (const auto& mi : fMsgInputs)
                {
                    for (unsigned int i = 0; i < fChannels.at(mi.first).size(); ++i)
                    {
                        if (poller->CheckInput(mi.first, i))
                        {
                            unique_ptr<FairMQMessage> msg(NewMessage());

                            if (Receive(msg, mi.first, i) >= 0)
                            {
                                if (mi.second(msg, i) == false)
                                {
                                    exitingRunningCallback = true;
                                    break;
                                }
                            }
                            else
                            {
                                exitingRunningCallback = true;
                                break;
                            }
                        }
                    }
                    if (exitingRunningCallback)
                    {
                        break;
                    }
                }

                for (const auto& mi : fMultipartInputs)
                {
                    for (unsigned int i = 0; i < fChannels.at(mi.first).size(); ++i)
                    {
                        if (poller->CheckInput(mi.first, i))
                        {
                            FairMQParts parts;

                            if (Receive(parts, mi.first, i) >= 0)
                            {
                                if (mi.second(parts, i) == false)
                                {
                                    exitingRunningCallback = true;
                                    break;
                                }
                            }
                            else
                            {
                                exitingRunningCallback = true;
                                break;
                            }
                        }
                    }
                    if (exitingRunningCallback)
                    {
                        break;
                    }
                }
            }
        }
        else
        {
            while (CheckCurrentState(RUNNING) && ConditionalRun())
            {
            }

            Run();
        }

        PostRun();
    }
    catch (const out_of_range& oor)
    {
        LOG(ERROR) << "Out of Range error: " << oor.what();
        LOG(ERROR) << "Incorrect channel name in the Run() or the configuration?";
        ChangeState(ERROR);
    }

    if (CheckCurrentState(RUNNING))
    {
        ChangeState(internal_READY);
    }

    try
    {
        rateLogger.join();
    }
    catch (exception& e)
    {
        LOG(ERROR) << "Exception cought during Run(): " << e.what();
        exit(EXIT_FAILURE);
    }
}

void FairMQDevice::Run()
{
}

void FairMQDevice::PreRun()
{
}

bool FairMQDevice::ConditionalRun()
{
    return false;
}

void FairMQDevice::PostRun()
{
}

void FairMQDevice::Pause()
{
    while (CheckCurrentState(PAUSED))
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        LOG(DEBUG) << "paused...";
    }
    LOG(DEBUG) << "Unpausing";
}

// Method for setting properties represented as a string.
void FairMQDevice::SetProperty(const int key, const string& value)
{
    switch (key)
    {
        case Id:
            fId = value;
            break;
        case NetworkInterface:
            fNetworkInterface = value;
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
        case MaxInitializationAttempts:
            fMaxInitializationAttempts = value;
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
        case NetworkInterface:
            return fNetworkInterface;
        default:
            return FairMQConfigurable::GetProperty(key, default_);
    }
}

string FairMQDevice::GetPropertyDescription(const int key)
{
    switch (key)
    {
        case Id:
            return "Id: Device ID";
        case NumIoThreads:
            return "NumIoThreads: Number of I/O Threads (size of the 0MQ thread pool to handle I/O operations. If your application is using only the inproc transport for messaging you may set this to zero, otherwise set it to at least one.)";
        case MaxInitializationAttempts:
            return "MaxInitializationAttempts: Maximum number of validation and initialization attempts of the channels.";
        case PortRangeMin:
            return "PortRangeMin: Minumum value for the port range (when binding to dynamic port).";
        case PortRangeMax:
            return "PortRangeMax: Maximum value for the port range (when binding to dynamic port).";
        case LogIntervalInMs:
            return "LogIntervalInMs: Time between socket rates logging outputs.";
        case NetworkInterface:
            return "NetworkInterface: Network interface to use for dynamic binding.";
        default:
            return FairMQConfigurable::GetPropertyDescription(key);
    }
}

void FairMQDevice::ListProperties()
{
    LOG(INFO) << "Properties of FairMQDevice:";
    for (int p = FairMQConfigurable::Last; p < FairMQDevice::Last; ++p)
    {
        LOG(INFO) << " " << GetPropertyDescription(p);
    }
    LOG(INFO) << "---------------------------";
}

// Method for getting properties represented as an integer.
int FairMQDevice::GetProperty(const int key, const int default_ /*= 0*/)
{
    switch (key)
    {
        case NumIoThreads:
            return fNumIoThreads;
        case MaxInitializationAttempts:
            return fMaxInitializationAttempts;
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
    fTransportFactory = unique_ptr<FairMQTransportFactory>(factory);
}

void FairMQDevice::SetTransport(const string& transport)
{
    if (transport == "zeromq")
    {
        fTransportFactory = unique_ptr<FairMQTransportFactory>(new FairMQTransportFactoryZMQ());
    }
#ifdef NANOMSG_FOUND
    else if (transport == "nanomsg")
    {
        fTransportFactory = unique_ptr<FairMQTransportFactory>(new FairMQTransportFactoryNN());
    }
#endif
    else
    {
        LOG(ERROR) << "Unavailable transport implementation requested: "
                   << "\"" << transport << "\""
                   << ". Available are: "
                   << "\"zeromq\""
#ifdef NANOMSG_FOUND
                   << ", \"nanomsg\""
#endif
                   << ". Exiting.";
        exit(EXIT_FAILURE);
    }
}

void FairMQDevice::SetConfig(FairMQProgOptions& config)
{
    LOG(DEBUG) << "PID: " << getpid();
    fConfig = &config;
    fChannels = config.GetFairMQMap();
    SetTransport(config.GetValue<string>("transport"));
    fId = config.GetValue<string>("id");
    fNetworkInterface = config.GetValue<string>("network-interface");
    fNumIoThreads = config.GetValue<int>("io-threads");
}

void FairMQDevice::LogSocketRates()
{
    timestamp_t t0;
    timestamp_t t1;

    timestamp_t msSinceLastLog;

    int numFilteredSockets = 0;
    vector<FairMQSocket*> filteredSockets;
    vector<string> filteredChannelNames;
    vector<int> logIntervals;
    vector<int> intervalCounters;

    // iterate over the channels map
    for (const auto& mi : fChannels)
    {
        // iterate over the channels vector
        for (auto vi = (mi.second).begin(); vi != (mi.second).end(); ++vi)
        {
            if (vi->fRateLogging > 0)
            {
                filteredSockets.push_back(vi->fSocket.get());
                logIntervals.push_back(vi->fRateLogging);
                intervalCounters.push_back(0);
                stringstream ss;
                ss << mi.first << "[" << vi - (mi.second).begin() << "]";
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
    for (const auto& vi : filteredSockets)
    {
        bytesIn.at(i) = vi->GetBytesRx();
        bytesOut.at(i) = vi->GetBytesTx();
        msgIn.at(i) = vi->GetMessagesRx();
        msgOut.at(i) = vi->GetMessagesTx();
        ++i;
    }

    t0 = get_timestamp();

    while (CheckCurrentState(RUNNING))
    {
        t1 = get_timestamp();

        msSinceLastLog = (t1 - t0) / 1000.0L;

        i = 0;

        for (const auto& vi : filteredSockets)
        {
            intervalCounters.at(i)++;

            if (intervalCounters.at(i) == logIntervals.at(i))
            {
                intervalCounters.at(i) = 0;

                bytesInNew.at(i) = vi->GetBytesRx();
                mbPerSecIn.at(i) = (static_cast<double>(bytesInNew.at(i) - bytesIn.at(i)) / (1024. * 1024.)) / static_cast<double>(msSinceLastLog) * 1000.;
                bytesIn.at(i) = bytesInNew.at(i);

                msgInNew.at(i) = vi->GetMessagesRx();
                msgPerSecIn.at(i) = static_cast<double>(msgInNew.at(i) - msgIn.at(i)) / static_cast<double>(msSinceLastLog) * 1000.;
                msgIn.at(i) = msgInNew.at(i);

                bytesOutNew.at(i) = vi->GetBytesTx();
                mbPerSecOut.at(i) = (static_cast<double>(bytesOutNew.at(i) - bytesOut.at(i)) / (1024. * 1024.)) / static_cast<double>(msSinceLastLog) * 1000.;
                bytesOut.at(i) = bytesOutNew.at(i);

                msgOutNew.at(i) = vi->GetMessagesTx();
                msgPerSecOut.at(i) = static_cast<double>(msgOutNew.at(i) - msgOut.at(i)) / static_cast<double>(msSinceLastLog) * 1000.;
                msgOut.at(i) = msgOutNew.at(i);

                LOG(DEBUG) << filteredChannelNames.at(i) << ": "
                           << "in: " << msgPerSecIn.at(i) << " msg (" << mbPerSecIn.at(i) << " MB), "
                           << "out: " << msgPerSecOut.at(i) << " msg (" << mbPerSecOut.at(i) << " MB)";
            }

            ++i;
        }

        t0 = t1;
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    // LOG(DEBUG) << "FairMQDevice::LogSocketRates() stopping";
}

void FairMQDevice::InteractiveStateLoop()
{
    fInteractiveRunning = true;
    char c; // hold the user console input
    pollfd cinfd[1];
    cinfd[0].fd = fileno(stdin);
    cinfd[0].events = POLLIN;

    struct termios t;
    tcgetattr(STDIN_FILENO, &t); // get the current terminal I/O structure
    t.c_lflag &= ~ICANON; // disable canonical input
    tcsetattr(STDIN_FILENO, TCSANOW, &t); // apply the new settings

    PrintInteractiveStateLoopHelp();

    while (fInteractiveRunning)
    {
        if (poll(cinfd, 1, 500))
        {
            if (!fInteractiveRunning)
            {
                break;
            }

            cin >> c;

            switch (c)
            {
                case 'i':
                    LOG(INFO) << "[i] init device";
                    ChangeState(INIT_DEVICE);
                    break;
                case 'j':
                    LOG(INFO) << "[j] init task";
                    ChangeState(INIT_TASK);
                    break;
                case 'p':
                    LOG(INFO) << "[p] pause";
                    ChangeState(PAUSE);
                    break;
                case 'r':
                    LOG(INFO) << "[r] run";
                    ChangeState(RUN);
                    break;
                case 's':
                    LOG(INFO) << "[s] stop";
                    ChangeState(STOP);
                    break;
                case 't':
                    LOG(INFO) << "[t] reset task";
                    ChangeState(RESET_TASK);
                    break;
                case 'd':
                    LOG(INFO) << "[d] reset device";
                    ChangeState(RESET_DEVICE);
                    break;
                case 'h':
                    LOG(INFO) << "[h] help";
                    PrintInteractiveStateLoopHelp();
                    break;
                // case 'x':
                //     LOG(INFO) << "[x] ERROR";
                //     ChangeState(ERROR_FOUND);
                //     break;
                case 'q':
                    LOG(INFO) << "[q] end";

                    ChangeState(STOP);

                    ChangeState(RESET_TASK);
                    WaitForEndOfState(RESET_TASK);

                    ChangeState(RESET_DEVICE);
                    WaitForEndOfState(RESET_DEVICE);

                    ChangeState(END);

                    if (CheckCurrentState(EXITING))
                    {
                        fInteractiveRunning = false;
                    }

                    LOG(INFO) << "Exiting.";
                    break;
                default:
                    LOG(INFO) << "Invalid input: [" << c << "]";
                    PrintInteractiveStateLoopHelp();
                    break;
            }
        }
    }

    tcgetattr(STDIN_FILENO, &t); // get the current terminal I/O structure
    t.c_lflag |= ICANON; // re-enable canonical input
    tcsetattr(STDIN_FILENO, TCSANOW, &t); // apply the new settings
}

void FairMQDevice::Unblock()
{
    FairMQChannel::fInterrupted = true;
    FairMQMessagePtr cmd(fTransportFactory->CreateMessage());
    fCmdSocket->Send(cmd.get(), 0);
}

void FairMQDevice::ResetTaskWrapper()
{
    ResetTask();

    ChangeState(internal_DEVICE_READY);
}

void FairMQDevice::ResetTask()
{
}

void FairMQDevice::ResetWrapper()
{
    Reset();

    ChangeState(internal_IDLE);
}

void FairMQDevice::Reset()
{
    // iterate over the channels map
    for (auto& mi : fChannels)
    {
        // iterate over the channels vector
        for (auto& vi : mi.second)
        {
            vi.fSocket->Close();
            vi.fSocket = nullptr;

            vi.fPoller = nullptr;

            vi.fCmdSocket->Close();
            vi.fCmdSocket = nullptr;
        }
    }
}

bool FairMQDevice::Terminated()
{
    return fTerminationRequested;
}

void FairMQDevice::Terminate()
{
    // Termination signal has to be sent only once to any socket.
    if (fCmdSocket)
    {
        fCmdSocket->Terminate();
    }
}

void FairMQDevice::Shutdown()
{
    LOG(DEBUG) << "Closing sockets...";

    // iterate over the channels map
    for (const auto& mi : fChannels)
    {
        // iterate over the channels vector
        for (const auto& vi : mi.second)
        {
            if (vi.fSocket)
            {
                vi.fSocket->Close();
            }
            if (vi.fCmdSocket)
            {
                vi.fCmdSocket->Close();
            }
        }
    }

    if (fCmdSocket)
    {
        fCmdSocket->Close();
    }

    LOG(DEBUG) << "Closed all sockets!";
}

FairMQDevice::~FairMQDevice()
{
    LOG(DEBUG) << "Device destroyed";
}
