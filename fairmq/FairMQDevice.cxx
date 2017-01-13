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
#include "FairMQTransportFactorySHM.h"
#ifdef NANOMSG_FOUND
#include "FairMQTransportFactoryNN.h"
#endif

using namespace std;

// function and a wrapper to catch the signals
function<void(int)> sigHandler;
static void CallSignalHandler(int signal)
{
    sigHandler(signal);
}

FairMQDevice::FairMQDevice()
    : fChannels()
    , fConfig(nullptr)
    , fId()
    , fNetworkInterface()
    , fDefaultTransport()
    , fMaxInitializationAttempts(120)
    , fNumIoThreads(1)
    , fPortRangeMin(22000)
    , fPortRangeMax(32000)
    , fLogIntervalInMs(1000)
    , fTransportFactory(nullptr)
    , fTransports()
    , fDeviceCmdSockets()
    , fInitialValidationFinished(false)
    , fInitialValidationCondition()
    , fInitialValidationMutex()
    , fCatchingSignals(false)
    , fTerminationRequested(false)
    , fInteractiveRunning(false)
    , fDataCallbacks(false)
    , fMsgInputs()
    , fMultipartInputs()
    , fMultitransportInputs()
    , fInputChannelKeys()
    , fMultitransportMutex()
    , fMultitransportProceed(false)
{
}

void FairMQDevice::CatchSignals()
{
    if (!fCatchingSignals)
    {
        sigHandler = bind1st(mem_fun(&FairMQDevice::SignalHandler), this);
        signal(SIGINT, CallSignalHandler);
        signal(SIGTERM, CallSignalHandler);
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
        abort();
        // exit(EXIT_FAILURE);
    }
}

void FairMQDevice::AttachChannels(list<FairMQChannel*>& chans)
{
    auto itr = chans.begin();

    while (itr != chans.end())
    {
        if ((*itr)->ValidateChannel())
        {
            if (AttachChannel(**itr))
            {
                (*itr)->InitCommandInterface(fNumIoThreads);
                chans.erase(itr++);
            }
            else
            {
                LOG(ERROR) << "failed to attach channel " << (*itr)->fName << " (" << (*itr)->fMethod << ")";
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

    if (fDeviceCmdSockets.empty())
    {
        auto p = fDeviceCmdSockets.emplace(fTransportFactory->GetType(), fTransportFactory->CreateSocket("pub", "device-commands", fNumIoThreads, fId));
        if (p.second)
        {
            p.first->second->Bind("inproc://commands");
        }
        else
        {
            exit(EXIT_FAILURE);
        }

        FairMQMessagePtr msg(fTransportFactory->CreateMessage());
        msg->SetDeviceId(fId);
    }

    // Containers to store the uninitialized channels.
    list<FairMQChannel*> uninitializedBindingChannels;
    list<FairMQChannel*> uninitializedConnectingChannels;

    // Fill the uninitialized channel containers
    for (auto mi = fChannels.begin(); mi != fChannels.end(); ++mi)
    {
        for (auto vi = (mi->second).begin(); vi != (mi->second).end(); ++vi)
        {
            // set channel name: name + vector index
            stringstream ss;
            ss << mi->first << "[" << vi - (mi->second).begin() << "]";
            vi->fName = ss.str();

            if (vi->fMethod == "bind")
            {
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
                // fill the uninitialized list
                uninitializedConnectingChannels.push_back(&(*vi));
            }
            else if (vi->fAddress.find_first_of("@+>") != string::npos)
            {
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
    AttachChannels(uninitializedBindingChannels);
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
        AttachChannels(uninitializedConnectingChannels);
        if (++numAttempts > fMaxInitializationAttempts)
        {
            LOG(ERROR) << "could not connect all channels after " << fMaxInitializationAttempts << " attempts";
            // TODO: goto ERROR state;
            exit(EXIT_FAILURE);
        }

        if (numAttempts != 0)
        {
            this_thread::sleep_for(chrono::milliseconds(1000));
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

bool FairMQDevice::AttachChannel(FairMQChannel& ch)
{
    if (!ch.fTransportFactory)
    {
        if (ch.fTransport == "default" || ch.fTransport == fDefaultTransport)
        {
            LOG(DEBUG) << ch.fName << ": using default transport";
            ch.InitTransport(fTransportFactory);
        }
        else
        {
            LOG(DEBUG) << ch.fName << ": channel transport (" << fDefaultTransport << ") overriden to " << ch.fTransport;
            ch.InitTransport(AddTransport(ch.fTransport));
        }
        ch.fTransportType = ch.fTransportFactory->GetType();
    }

    vector<string> endpoints;
    FairMQChannel::Tokenize(endpoints, ch.fAddress);
    for (auto& endpoint : endpoints)
    {
        //(re-)init socket
        if (!ch.fSocket)
        {
            ch.fSocket = ch.fTransportFactory->CreateSocket(ch.fType, ch.fName, fNumIoThreads, fId);
        }

        // set high water marks
        ch.fSocket->SetOption("snd-hwm", &(ch.fSndBufSize), sizeof(ch.fSndBufSize));
        ch.fSocket->SetOption("rcv-hwm", &(ch.fRcvBufSize), sizeof(ch.fRcvBufSize));

        // set kernel transmit size
        if (ch.fSndKernelSize != 0)
        {
            ch.fSocket->SetOption("snd-size", &(ch.fSndKernelSize), sizeof(ch.fSndKernelSize));
        }
        if (ch.fRcvKernelSize != 0)
        {
            ch.fSocket->SetOption("rcv-size", &(ch.fRcvKernelSize), sizeof(ch.fRcvKernelSize));
        }

        // attach
        bool bind = (ch.fMethod == "bind");
        bool connectionModifier = false;
        string address = endpoint;

        // check if the default fMethod is overridden by a modifier
        if (endpoint[0] == '+' || endpoint[0] == '>')
        {
            connectionModifier = true;
            bind = false;
            address = endpoint.substr(1);
        }
        else if (endpoint[0] == '@')
        {
            connectionModifier = true;
            bind = true;
            address = endpoint.substr(1);
        }

        bool rc = true;
        // make the connection
        if (bind)
        {
            rc = BindEndpoint(*ch.fSocket, address);
        }
        else
        {
            rc = ConnectEndpoint(*ch.fSocket, address);
        }

        // bind might bind to an address different than requested,
        // put the actual address back in the config
        endpoint.clear();
        if (connectionModifier)
        {
            endpoint.push_back(bind?'@':'+');
        }
        endpoint += address;

        LOG(DEBUG) << "Attached channel " << ch.fName << " to " << endpoint << (bind ? " (bind) " : " (connect) ");

        // after the book keeping is done, exit in case of errors
        if (!rc)
        {
            return rc;
        }
    }

    // put the (possibly) modified address back in the config
    ch.UpdateAddress(boost::algorithm::join(endpoints, ","));

    return true;
}

bool FairMQDevice::ConnectEndpoint(FairMQSocket& socket, string& endpoint)
{
    socket.Connect(endpoint);

    return true;
}

bool FairMQDevice::BindEndpoint(FairMQSocket& socket, string& endpoint)
{
    // number of attempts when choosing a random port
    int maxAttempts = 1000;
    int numAttempts = 0;

    // initialize random generator
    default_random_engine generator(chrono::system_clock::now().time_since_epoch().count());
    uniform_int_distribution<int> randomPort(fPortRangeMin, fPortRangeMax);

    // try to bind to the saved port. In case of failure, try random one.
    while (!socket.Bind(endpoint))
    {
        LOG(DEBUG) << "Could not bind to configured (TCP) port, trying random port in range " << fPortRangeMin << "-" << fPortRangeMax;
        ++numAttempts;

        if (numAttempts > maxAttempts)
        {
            LOG(ERROR) << "could not bind to any (TCP) port in the given range after " << maxAttempts << " attempts";
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
                vi->fName = ss.str();
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
            LOG(INFO) << vi->fName << ": "
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

    if (find(fInputChannelKeys.begin(), fInputChannelKeys.end(), channelName) == fInputChannelKeys.end())
    {
        fInputChannelKeys.push_back(channelName);
    }
}

void FairMQDevice::OnData(const string& channelName, InputMultipartCallback callback)
{
    fDataCallbacks = true;
    fMultipartInputs.insert(make_pair(channelName, callback));

    if (find(fInputChannelKeys.begin(), fInputChannelKeys.end(), channelName) == fInputChannelKeys.end())
    {
        fInputChannelKeys.push_back(channelName);
    }
}

void FairMQDevice::RunWrapper()
{
    LOG(INFO) << "DEVICE: Running...";

    // start the rate logger thread
    thread rateLogger(&FairMQDevice::LogSocketRates, this);

    // notify channels to resume transfers
    FairMQChannel::fInterrupted = false;
    for (auto& kv : fDeviceCmdSockets)
    {
        kv.second->Resume();
    }

    try
    {
        PreRun();

        // process either data callbacks or ConditionalRun/Run
        if (fDataCallbacks)
        {
            // if only one input channel, do lightweight handling without additional polling.
            if (fInputChannelKeys.size() == 1 && fChannels.at(fInputChannelKeys.at(0)).size() == 1)
            {
                HandleSingleChannelInput();
            }
            else // otherwise do full handling with polling
            {
                HandleMultipleChannelInput();
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
        LOG(ERROR) << "out of range: " << oor.what();
        LOG(ERROR) << "incorrect/incomplete channel configuration?";
    }

    // if Run() exited and the state is still RUNNING, transition to READY.
    if (CheckCurrentState(RUNNING))
    {
        ChangeState(internal_READY);
    }

    rateLogger.join();
}

void FairMQDevice::HandleSingleChannelInput()
{
    bool proceed = true;

    if (fMsgInputs.size() > 0)
    {
        while (CheckCurrentState(RUNNING) && proceed)
        {
            proceed = HandleMsgInput(fInputChannelKeys.at(0), fMsgInputs.begin()->second, 0);
        }
    }
    else if (fMultipartInputs.size() > 0)
    {
        while (CheckCurrentState(RUNNING) && proceed)
        {
            proceed = HandleMultipartInput(fInputChannelKeys.at(0), fMultipartInputs.begin()->second, 0);
        }
    }
}

void FairMQDevice::HandleMultipleChannelInput()
{
    // check if more than one transport is used
    fMultitransportInputs.clear();
    for (const auto& k : fInputChannelKeys)
    {
        FairMQ::Transport t = fChannels.at(k).at(0).fTransportType;
        if (fMultitransportInputs.find(t) == fMultitransportInputs.end())
        {
            fMultitransportInputs.insert(pair<FairMQ::Transport, vector<string>>(t, vector<string>()));
            fMultitransportInputs.at(t).push_back(k);
        }
        else
        {
            fMultitransportInputs.at(t).push_back(k);
        }
    }

    for (const auto& mi : fMsgInputs)
    {
        for (unsigned int i = 0; i < fChannels.at(mi.first).size(); ++i)
        {
            fChannels.at(mi.first).at(i).fMultipart = false;
        }
    }

    for (const auto& mi : fMultipartInputs)
    {
        for (unsigned int i = 0; i < fChannels.at(mi.first).size(); ++i)
        {
            fChannels.at(mi.first).at(i).fMultipart = true;
        }
    }

    // if more than one transport is used, handle poll of each in a separate thread
    if (fMultitransportInputs.size() > 1)
    {
        HandleMultipleTransportInput();
    }
    else // otherwise poll directly
    {
        bool proceed = true;

        FairMQPollerPtr poller(fChannels.at(fInputChannelKeys.at(0)).at(0).fTransportFactory->CreatePoller(fChannels, fInputChannelKeys));

        while (CheckCurrentState(RUNNING) && proceed)
        {
            poller->Poll(200);

            // check which inputs are ready and call their data handlers if they are.
            for (const auto& ch : fInputChannelKeys)
            {
                for (unsigned int i = 0; i < fChannels.at(ch).size(); ++i)
                {
                    if (poller->CheckInput(ch, i))
                    {
                        if (fChannels.at(ch).at(i).fMultipart)
                        {
                            proceed = HandleMultipartInput(ch, fMultipartInputs.at(ch), i);
                        }
                        else
                        {
                            proceed = HandleMsgInput(ch, fMsgInputs.at(ch), i);
                        }

                        if (!proceed)
                        {
                            break;
                        }
                    }
                }
                if (!proceed)
                {
                    break;
                }
            }
        }
    }
}

void FairMQDevice::HandleMultipleTransportInput()
{
    vector<thread> threads;

    fMultitransportProceed = true;

    for (const auto& i : fMultitransportInputs)
    {
        threads.push_back(thread(&FairMQDevice::PollForTransport, this, fTransports.at(i.first).get(), i.second));
    }

    for (thread& t : threads)
    {
        t.join();
    }
}

void FairMQDevice::PollForTransport(const FairMQTransportFactory* factory, const vector<string>& channelKeys)
{
    try
    {
        FairMQPollerPtr poller(factory->CreatePoller(fChannels, channelKeys));

        while (CheckCurrentState(RUNNING) && fMultitransportProceed)
        {
            poller->Poll(500);

            for (const auto& ch : channelKeys)
            {
                for (unsigned int i = 0; i < fChannels.at(ch).size(); ++i)
                {
                    if (poller->CheckInput(ch, i))
                    {
                        lock_guard<mutex> lock(fMultitransportMutex);

                        if (!fMultitransportProceed)
                        {
                            break;
                        }

                        if (fChannels.at(ch).at(i).fMultipart)
                        {
                            fMultitransportProceed = HandleMultipartInput(ch, fMultipartInputs.at(ch), i);
                        }
                        else
                        {
                            fMultitransportProceed = HandleMsgInput(ch, fMsgInputs.at(ch), i);
                        }

                        if (!fMultitransportProceed)
                        {
                            break;
                        }
                    }
                }
                if (!fMultitransportProceed)
                {
                    break;
                }
            }
        }
    }
    catch (std::exception& e)
    {
        LOG(ERROR) << "FairMQDevice::PollForTransport() failed: " << e.what() << ", going to ERROR state.";
        ChangeState(ERROR_FOUND);
    }
}

bool FairMQDevice::HandleMsgInput(const string& chName, const InputMsgCallback& callback, int i) const
{
    unique_ptr<FairMQMessage> input(fChannels.at(chName).at(i).fTransportFactory->CreateMessage());

    if (Receive(input, chName, i) >= 0)
    {
        return callback(input, 0);
    }
    else
    {
        return false;
    }
}

bool FairMQDevice::HandleMultipartInput(const string& chName, const InputMultipartCallback& callback, int i) const
{
    FairMQParts input;

    if (Receive(input, chName, i) >= 0)
    {
        return callback(input, 0);
    }
    else
    {
        return false;
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
        this_thread::sleep_for(chrono::milliseconds(500));
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

// DEPRECATED, use the string version
void FairMQDevice::SetTransport(FairMQTransportFactory* factory)
{
    if (fTransports.empty())
    {
        fTransportFactory = shared_ptr<FairMQTransportFactory>(factory);
        pair<FairMQ::Transport, shared_ptr<FairMQTransportFactory>> t(fTransportFactory->GetType(), fTransportFactory);
        fTransports.insert(t);
    }
    else
    {
        LOG(ERROR) << "Transports container is not empty when setting transport. Setting twice?";
        ChangeState(ERROR_FOUND);
    }
}

shared_ptr<FairMQTransportFactory> FairMQDevice::AddTransport(const string& transport)
{
    unordered_map<FairMQ::Transport, shared_ptr<FairMQTransportFactory>>::const_iterator i = fTransports.find(FairMQ::TransportTypes.at(transport));

    if (i == fTransports.end())
    {
        shared_ptr<FairMQTransportFactory> tr;

        if (transport == "zeromq")
        {
            tr = make_shared<FairMQTransportFactoryZMQ>();
        }
        else if (transport == "shmem")
        {
            tr = make_shared<FairMQTransportFactorySHM>();
        }
#ifdef NANOMSG_FOUND
        else if (transport == "nanomsg")
        {
            tr = make_shared<FairMQTransportFactoryNN>();
        }
#endif
        else
        {
            LOG(ERROR) << "Unavailable transport requested: "
                       << "\"" << transport << "\""
                       << ". Available are: "
                       << "\"zeromq\""
                       << "\"shmem\""
#ifdef NANOMSG_FOUND
                       << ", \"nanomsg\""
#endif
                       << ". Exiting.";
            exit(EXIT_FAILURE);
        }

        LOG(DEBUG) << "Adding '" << transport << "' transport to the device.";

        pair<FairMQ::Transport, shared_ptr<FairMQTransportFactory>> trPair(FairMQ::TransportTypes.at(transport), tr);
        fTransports.insert(trPair);

        auto p = fDeviceCmdSockets.emplace(tr->GetType(), tr->CreateSocket("pub", "device-commands", fNumIoThreads, fId));
        if (p.second)
        {
            p.first->second->Bind("inproc://commands");
        }
        else
        {
            exit(EXIT_FAILURE);
        }

        FairMQMessagePtr msg(tr->CreateMessage());
        msg->SetDeviceId(fId);

        return move(tr);
    }
    else
    {
        LOG(DEBUG) << "Reusing existing '" << transport << "' transport.";
        return i->second;
    }
}

void FairMQDevice::SetTransport(const string& transport)
{
    if (fTransports.empty())
    {
        LOG(DEBUG) << "Requesting '" << transport << "' as default transport for the device";
        fTransportFactory = AddTransport(transport);
    }
    else
    {
        LOG(ERROR) << "Transports container is not empty when setting transport. Setting default twice?";
        ChangeState(ERROR_FOUND);
    }
}

void FairMQDevice::SetConfig(FairMQProgOptions& config)
{
    LOG(DEBUG) << "PID: " << getpid();
    fConfig = &config;
    fChannels = config.GetFairMQMap();
    fDefaultTransport = config.GetValue<string>("transport");
    SetTransport(fDefaultTransport);
    fId = config.GetValue<string>("id");
    fNetworkInterface = config.GetValue<string>("network-interface");
    fNumIoThreads = config.GetValue<int>("io-threads");
}

void FairMQDevice::LogSocketRates()
{
    timestamp_t t0;
    timestamp_t t1;

    timestamp_t msSinceLastLog;

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
            }
        }
    }

    unsigned int numFilteredSockets = filteredSockets.size();

    if (numFilteredSockets > 0)
    {
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
                    mbPerSecIn.at(i) = (static_cast<double>(bytesInNew.at(i) - bytesIn.at(i)) / (1000. * 1000.)) / static_cast<double>(msSinceLastLog) * 1000.;
                    bytesIn.at(i) = bytesInNew.at(i);

                    msgInNew.at(i) = vi->GetMessagesRx();
                    msgPerSecIn.at(i) = static_cast<double>(msgInNew.at(i) - msgIn.at(i)) / static_cast<double>(msSinceLastLog) * 1000.;
                    msgIn.at(i) = msgInNew.at(i);

                    bytesOutNew.at(i) = vi->GetBytesTx();
                    mbPerSecOut.at(i) = (static_cast<double>(bytesOutNew.at(i) - bytesOut.at(i)) / (1000. * 1000.)) / static_cast<double>(msSinceLastLog) * 1000.;
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
            this_thread::sleep_for(chrono::milliseconds(1000));
        }
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
    for (auto& kv : fDeviceCmdSockets)
    {
        kv.second->Interrupt();
        FairMQMessagePtr cmd(fTransports.at(kv.first)->CreateMessage());
        kv.second->Send(cmd);
    }
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

            vi.fChannelCmdSocket->Close();
            vi.fChannelCmdSocket = nullptr;
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
    for (auto& kv : fDeviceCmdSockets)
    {
        kv.second->Terminate();
    }
    // if (!fDeviceCmdSockets.empty())
    // {
    //     fDeviceCmdSockets[0]->Terminate();
    // }
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
            if (vi.fChannelCmdSocket)
            {
                vi.fChannelCmdSocket->Close();
            }
        }
    }

    for (auto& s : fDeviceCmdSockets)
    {
        s.second->Close();
    }

    // if (!fDeviceCmdSockets.empty())
    // {
    //     fDeviceCmdSockets[0]->Close();
    // }

    LOG(DEBUG) << "Closed all sockets!";
}

FairMQDevice::~FairMQDevice()
{
    LOG(DEBUG) << "Device destroyed";
}
