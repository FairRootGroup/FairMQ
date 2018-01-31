/********************************************************************************
 * Copyright (C) 2012-2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <list>
#include <cstdlib>
#include <stdexcept>
#include <random>
#include <chrono>
#include <mutex>
#include <thread>
#include <functional>

#include <boost/algorithm/string.hpp> // join/split

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "FairMQSocket.h"
#include "FairMQDevice.h"
#include "FairMQLogger.h"
#include <fairmq/Tools.h>

#include "options/FairMQProgOptions.h"
#include "zeromq/FairMQTransportFactoryZMQ.h"
#include "shmem/FairMQTransportFactorySHM.h"
#ifdef NANOMSG_FOUND
#include "nanomsg/FairMQTransportFactoryNN.h"
#endif

using namespace std;

FairMQDevice::FairMQDevice()
    : fTransportFactory(nullptr)
    , fTransports()
    , fChannels()
    , fConfig(nullptr)
    , fId()
    , fNumIoThreads(1)
    , fInitialValidationFinished(false)
    , fInitialValidationCondition()
    , fInitialValidationMutex()
    , fPortRangeMin(22000)
    , fPortRangeMax(32000)
    , fNetworkInterface()
    , fDefaultTransport()
    , fInitializationTimeoutInS(120)
    , fDataCallbacks(false)
    , fDeviceCmdSockets()
    , fMsgInputs()
    , fMultipartInputs()
    , fMultitransportInputs()
    , fChannelRegistry()
    , fInputChannelKeys()
    , fMultitransportMutex()
    , fMultitransportProceed(false)
    , fExternalConfig(false)
    , fVersion({0, 0, 0})
    , fRate(0.)
    , fLastTime(0)
{
}

FairMQDevice::FairMQDevice(const fair::mq::tools::Version version)
    : fTransportFactory(nullptr)
    , fTransports()
    , fChannels()
    , fConfig(nullptr)
    , fId()
    , fNumIoThreads(1)
    , fInitialValidationFinished(false)
    , fInitialValidationCondition()
    , fInitialValidationMutex()
    , fPortRangeMin(22000)
    , fPortRangeMax(32000)
    , fNetworkInterface()
    , fDefaultTransport()
    , fInitializationTimeoutInS(120)
    , fDataCallbacks(false)
    , fDeviceCmdSockets()
    , fMsgInputs()
    , fMultipartInputs()
    , fMultitransportInputs()
    , fChannelRegistry()
    , fInputChannelKeys()
    , fMultitransportMutex()
    , fMultitransportProceed(false)
    , fExternalConfig(false)
    , fVersion(version)
    , fRate(0.)
    , fLastTime(0)
{
}

void FairMQDevice::InitWrapper()
{
    if (!fTransportFactory)
    {
        LOG(error) << "Transport not initialized. Did you call SetTransport()?";
        exit(EXIT_FAILURE);
    }

    if (fDeviceCmdSockets.empty())
    {
        auto p = fDeviceCmdSockets.emplace(fTransportFactory->GetType(), fTransportFactory->CreateSocket("pub", "device-commands"));
        if (p.second)
        {
            p.first->second->Bind("inproc://commands");
        }
        else
        {
            exit(EXIT_FAILURE);
        }

        FairMQMessagePtr msg(fTransportFactory->CreateMessage());
    }

    // Containers to store the uninitialized channels.
    vector<FairMQChannel*> uninitializedBindingChannels;
    vector<FairMQChannel*> uninitializedConnectingChannels;

    // Fill the uninitialized channel containers
    for (auto& mi : fChannels)
    {
        for (auto vi = mi.second.begin(); vi != mi.second.end(); ++vi)
        {
            // if (vi->fModified)
            // {
                // if (vi->fReset)
                // {
                //     vi->fSocket.reset();
                //     vi->fPoller.reset();
                //     vi->fChannelCmdSocket.reset();
                // }
                // set channel name: name + vector index
                vi->fName = fair::mq::tools::ToString(mi.first, "[", vi - (mi.second).begin(), "]");

                if (vi->fMethod == "bind")
                {
                    // if binding address is not specified, try getting it from the configured network interface
                    if (vi->fAddress == "unspecified" || vi->fAddress == "")
                    {
                        // if the configured network interface is default, get its name from the default route
                        if (fNetworkInterface == "default")
                        {
                            fNetworkInterface = fair::mq::tools::getDefaultRouteNetworkInterface();
                        }
                        vi->fAddress = "tcp://" + fair::mq::tools::getInterfaceIP(fNetworkInterface) + ":1";
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
                    LOG(error) << "Cannot update configuration. Socket method (bind/connect) not specified.";
                    throw runtime_error("Cannot update configuration. Socket method (bind/connect) not specified.");
                }
            // }
        }
    }

    // Bind channels. Here one run is enough, because bind settings should be available locally
    // If necessary this could be handled in the same way as the connecting channels
    AttachChannels(uninitializedBindingChannels);

    if (uninitializedBindingChannels.size() > 0)
    {
        LOG(error) << uninitializedBindingChannels.size() << " of the binding channels could not initialize. Initial configuration incomplete.";
        throw runtime_error(fair::mq::tools::ToString(uninitializedBindingChannels.size(), " of the binding channels could not initialize. Initial configuration incomplete."));
    }

    CallStateChangeCallbacks(INITIALIZING_DEVICE);

    // notify parent thread about completion of first validation.
    {
        lock_guard<mutex> lock(fInitialValidationMutex);
        fInitialValidationFinished = true;
        fInitialValidationCondition.notify_one();
    }

    // go over the list of channels until all are initialized (and removed from the uninitialized list)
    int numAttempts = 1;
    auto sleepTimeInMS = 50;
    auto maxAttempts = fInitializationTimeoutInS * 1000 / sleepTimeInMS;
    // first attempt
    AttachChannels(uninitializedConnectingChannels);
    // if not all channels could be connected, update their address values from config and retry
    while (!uninitializedConnectingChannels.empty())
    {
        this_thread::sleep_for(chrono::milliseconds(sleepTimeInMS));

        if (fConfig)
        {
            for (auto& chan : uninitializedConnectingChannels)
            {
                string key{"chans." + chan->GetChannelPrefix() + "." + chan->GetChannelIndex() + ".address"};
                string newAddress = fConfig->GetValue<string>(key);
                if (newAddress != chan->GetAddress())
                {
                    chan->UpdateAddress(newAddress);
                }
            }
        }

        if (numAttempts++ > maxAttempts)
        {
            LOG(error) << "could not connect all channels after " << fInitializationTimeoutInS << " attempts";
            throw runtime_error(fair::mq::tools::ToString("could not connect all channels after ", fInitializationTimeoutInS, " attempts"));
        }

        AttachChannels(uninitializedConnectingChannels);
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

void FairMQDevice::AttachChannels(vector<FairMQChannel*>& chans)
{
    auto itr = chans.begin();

    while (itr != chans.end())
    {
        if ((*itr)->ValidateChannel())
        {
            if (AttachChannel(**itr))
            {
                (*itr)->InitCommandInterface();
                (*itr)->SetModified(false);
                itr = chans.erase(itr);
            }
            else
            {
                LOG(error) << "failed to attach channel " << (*itr)->fName << " (" << (*itr)->fMethod << ")";
                ++itr;
            }
        }
        else
        {
            ++itr;
        }
    }
}

bool FairMQDevice::AttachChannel(FairMQChannel& ch)
{
    if (!ch.fTransportFactory)
    {
        if (ch.fTransport == "default" || ch.fTransport == fDefaultTransport)
        {
            LOG(debug) << ch.fName << ": using default transport";
            ch.InitTransport(fTransportFactory);
        }
        else
        {
            LOG(debug) << ch.fName << ": channel transport (" << fDefaultTransport << ") overriden to " << ch.fTransport;
            ch.InitTransport(AddTransport(ch.fTransport));
        }
        ch.fTransportType = ch.fTransportFactory->GetType();
    }

    vector<string> endpoints;
    boost::algorithm::split(endpoints, ch.fAddress, boost::algorithm::is_any_of(","));
    for (auto& endpoint : endpoints)
    {
        //(re-)init socket
        if (!ch.fSocket)
        {
            ch.fSocket = ch.fTransportFactory->CreateSocket(ch.fType, ch.fName);
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

        LOG(debug) << "Attached channel " << ch.fName << " to " << endpoint << (bind ? " (bind) " : " (connect) ");

        // after the book keeping is done, exit in case of errors
        if (!rc)
        {
            return rc;
        }
    }

    // put the (possibly) modified address back in the channel object and config
    string newAddress{boost::algorithm::join(endpoints, ",")};
    if (newAddress != ch.fAddress)
    {
        ch.UpdateAddress(newAddress);
        if (fConfig)
        {
            string key{"chans." + ch.GetChannelPrefix() + "." + ch.GetChannelIndex() + ".address"};
            fConfig->SetValue(key, newAddress);
        }
    }

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
        LOG(debug) << "Could not bind to configured (TCP) port, trying random port in range " << fPortRangeMin << "-" << fPortRangeMax;
        ++numAttempts;

        if (numAttempts > maxAttempts)
        {
            LOG(error) << "could not bind to any (TCP) port in the given range after " << maxAttempts << " attempts";
            return false;
        }

        size_t pos = endpoint.rfind(":");
        endpoint = endpoint.substr(0, pos + 1) + fair::mq::tools::ToString(static_cast<int>(randomPort(generator)));
    }

    return true;
}

void FairMQDevice::InitTaskWrapper()
{
    CallStateChangeCallbacks(INITIALIZING_TASK);

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
                vi->fName = fair::mq::tools::ToString(name, "[", vi - fChannels.at(name).begin(), "]");
            }
        }
    }
    else
    {
        LOG(error) << "Sorting failed: no channel with the name \"" << name << "\".";
    }
}

void FairMQDevice::PrintChannel(const string& name)
{
    if (fChannels.find(name) != fChannels.end())
    {
        for (auto vi = fChannels[name].begin(); vi != fChannels[name].end(); ++vi)
        {
            LOG(info) << vi->fName << ": "
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
        LOG(error) << "Printing failed: no channel with the name \"" << name << "\".";
    }
}

void FairMQDevice::RunWrapper()
{
    CallStateChangeCallbacks(RUNNING);

    LOG(info) << "DEVICE: Running...";

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
            using Clock = chrono::steady_clock;
            using TimeScale = chrono::microseconds;
            const TimeScale::rep period = TimeScale::period::den / fRate;
            const auto reftime = Clock::now();
            while (CheckCurrentState(RUNNING) && ConditionalRun())
            {
              if (fRate > 0.001) {
                auto timespan = chrono::duration_cast<TimeScale>(Clock::now() - reftime).count() - fLastTime;
                if (timespan < period) {
                  TimeScale sleepfor(period - timespan);
                  this_thread::sleep_for(sleepfor);
                }
                fLastTime = chrono::duration_cast<TimeScale>(Clock::now() - reftime).count();
              }
            }

            Run();
        }
    }
    catch (const out_of_range& oor)
    {
        LOG(error) << "out of range: " << oor.what();
        LOG(error) << "incorrect/incomplete channel configuration?";
    }

    // if Run() exited and the state is still RUNNING, transition to READY.
    if (CheckCurrentState(RUNNING))
    {
        ChangeState(internal_READY);
    }

    PostRun();

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
    catch (exception& e)
    {
        LOG(error) << "FairMQDevice::PollForTransport() failed: " << e.what() << ", going to ERROR state.";
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

void FairMQDevice::PauseWrapper()
{
    CallStateChangeCallbacks(PAUSED);

    Pause();
}

void FairMQDevice::Pause()
{
    while (CheckCurrentState(PAUSED))
    {
        this_thread::sleep_for(chrono::milliseconds(500));
        LOG(debug) << "paused...";
    }
    LOG(debug) << "Unpausing";
}

shared_ptr<FairMQTransportFactory> FairMQDevice::AddTransport(const string& transport)
{
    auto i = fTransports.find(FairMQ::TransportTypes.at(transport));

    if (i == fTransports.end())
    {
        auto tr = FairMQTransportFactory::CreateTransportFactory(transport, fId, fConfig);

        LOG(debug) << "Adding '" << transport << "' transport to the device.";

        pair<FairMQ::Transport, shared_ptr<FairMQTransportFactory>> trPair(FairMQ::TransportTypes.at(transport), tr);
        fTransports.insert(trPair);

        auto p = fDeviceCmdSockets.emplace(tr->GetType(), tr->CreateSocket("pub", "device-commands"));
        if (p.second)
        {
            p.first->second->Bind("inproc://commands");
        }
        else
        {
            exit(EXIT_FAILURE);
        }

        FairMQMessagePtr msg(tr->CreateMessage());

        return tr;
    }
    else
    {
        LOG(debug) << "Reusing existing '" << transport << "' transport.";
        return i->second;
    }
}

unique_ptr<FairMQTransportFactory> FairMQDevice::MakeTransport(const string& transport)
{
    unique_ptr<FairMQTransportFactory> tr;

    if (transport == "zeromq")
    {
        tr = fair::mq::tools::make_unique<FairMQTransportFactoryZMQ>();
    }
    else if (transport == "shmem")
    {
        tr = fair::mq::tools::make_unique<FairMQTransportFactorySHM>();
    }
#ifdef NANOMSG_FOUND
    else if (transport == "nanomsg")
    {
        tr = fair::mq::tools::make_unique<FairMQTransportFactoryNN>();
    }
#endif
    else
    {
        LOG(error) << "Unavailable transport requested: " << "\"" << transport << "\"" << ". Available are: "
                   << "\"zeromq\""
                   << "\"shmem\""
#ifdef NANOMSG_FOUND
                   << ", \"nanomsg\""
#endif
                   << ". Returning nullptr.";
        return tr;
    }

    return tr;
}

void FairMQDevice::CreateOwnConfig()
{
    // TODO: make fConfig a shared_ptr when no old user code has FairMQProgOptions ptr*
    fConfig = new FairMQProgOptions();

    string id{boost::uuids::to_string(boost::uuids::random_generator()())};
    LOG(warn) << "No FairMQProgOptions provided, creating one internally and setting device ID to " << id;

    // dummy argc+argv
    char arg0[] = "undefined"; // executable name
    char arg1[] = "--id";
    char* arg2 = const_cast<char*>(id.c_str()); // device ID
    const char* argv[] = { &arg0[0], &arg1[0], arg2, nullptr };
    int argc = static_cast<int>((sizeof(argv) / sizeof(argv[0])) - 1);

    fConfig->ParseAll(argc, &argv[0]);

    fId = fConfig->GetValue<string>("id");
    fNetworkInterface = fConfig->GetValue<string>("network-interface");
    fNumIoThreads = fConfig->GetValue<int>("io-threads");
    fInitializationTimeoutInS = fConfig->GetValue<int>("initialization-timeout");
}

void FairMQDevice::SetTransport(const string& transport)
{
    // This method is the first to be called, if FairMQProgOptions are not used (either SetTransport() or SetConfig() make sense, not both).
    // Make sure here that at least internal config is available.
    if (!fExternalConfig && !fConfig)
    {
        CreateOwnConfig();
    }

    if (fTransports.empty())
    {
        LOG(debug) << "Requesting '" << transport << "' as default transport for the device";
        fTransportFactory = AddTransport(transport);
    }
    else
    {
        LOG(error) << "Transports container is not empty when setting transport. Setting default twice?";
        ChangeState(ERROR_FOUND);
    }
}

void FairMQDevice::SetConfig(FairMQProgOptions& config)
{
    fExternalConfig = true;
    fConfig = &config;
    for (auto& c : config.GetFairMQMap())
    {
        if (!fChannels.insert(c).second)
        {
            LOG(warn) << "did not insert channel '" << c.first << "', it is already in the device.";
        }
    }
    fDefaultTransport = config.GetValue<string>("transport");
    SetTransport(fDefaultTransport);
    fId = config.GetValue<string>("id");
    fNetworkInterface = config.GetValue<string>("network-interface");
    fNumIoThreads = config.GetValue<int>("io-threads");
    fInitializationTimeoutInS = config.GetValue<int>("initialization-timeout");
    fRate = fConfig->GetValue<float>("rate");
}

void FairMQDevice::LogSocketRates()
{
    chrono::time_point<chrono::high_resolution_clock> t0;
    chrono::time_point<chrono::high_resolution_clock> t1;

    unsigned long long msSinceLastLog;

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
                filteredChannelNames.push_back(fair::mq::tools::ToString(mi.first, "[", vi - (mi.second).begin(), "]"));
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

        t0 = chrono::high_resolution_clock::now();

        LOG(debug) << "<channel>: in: <#msgs> (<MB>) out: <#msgs> (<MB>)";

        while (CheckCurrentState(RUNNING))
        {
            t1 = chrono::high_resolution_clock::now();

            msSinceLastLog = chrono::duration_cast<chrono::milliseconds>(t1 - t0).count();

            i = 0;

            for (const auto& vi : filteredSockets)
            {
                intervalCounters.at(i)++;

                if (intervalCounters.at(i) == logIntervals.at(i))
                {
                    intervalCounters.at(i) = 0;

                    bytesInNew.at(i) = vi->GetBytesRx();
                    msgInNew.at(i) = vi->GetMessagesRx();
                    bytesOutNew.at(i) = vi->GetBytesTx();
                    msgOutNew.at(i) = vi->GetMessagesTx();

                    mbPerSecIn.at(i) = (static_cast<double>(bytesInNew.at(i) - bytesIn.at(i)) / (1000. * 1000.)) / static_cast<double>(msSinceLastLog) * 1000.;
                    msgPerSecIn.at(i) = static_cast<double>(msgInNew.at(i) - msgIn.at(i)) / static_cast<double>(msSinceLastLog) * 1000.;
                    mbPerSecOut.at(i) = (static_cast<double>(bytesOutNew.at(i) - bytesOut.at(i)) / (1000. * 1000.)) / static_cast<double>(msSinceLastLog) * 1000.;
                    msgPerSecOut.at(i) = static_cast<double>(msgOutNew.at(i) - msgOut.at(i)) / static_cast<double>(msSinceLastLog) * 1000.;

                    bytesIn.at(i) = bytesInNew.at(i);
                    msgIn.at(i) = msgInNew.at(i);
                    bytesOut.at(i) = bytesOutNew.at(i);
                    msgOut.at(i) = msgOutNew.at(i);

                    LOG(debug) << filteredChannelNames.at(i) << ": "
                               << "in: " << msgPerSecIn.at(i) << " (" << mbPerSecIn.at(i) << " MB) "
                               << "out: " << msgPerSecOut.at(i) << " (" << mbPerSecOut.at(i) << " MB)";
                }

                ++i;
            }

            t0 = t1;
            this_thread::sleep_for(chrono::milliseconds(1000));
        }
    }
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
    CallStateChangeCallbacks(RESETTING_TASK);

    ResetTask();

    ChangeState(internal_DEVICE_READY);
}

void FairMQDevice::ResetTask()
{
}

void FairMQDevice::ResetWrapper()
{
    CallStateChangeCallbacks(RESETTING_DEVICE);

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
            // vi.fReset = true;
            vi.fSocket.reset();
            vi.fPoller.reset();
            vi.fChannelCmdSocket.reset();
        }
    }
}

const FairMQChannel& FairMQDevice::GetChannel(const string& channelName, const int index) const
{
    return fChannels.at(channelName).at(index);
}

void FairMQDevice::Exit()
{
    if (!fExternalConfig && fConfig)
    {
        delete fConfig;
    }
}

FairMQDevice::~FairMQDevice()
{
    LOG(debug) << "Destructing device " << fId;
}
