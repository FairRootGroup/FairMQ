/********************************************************************************
 * Copyright (C) 2012-2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <FairMQDevice.h>

#include <fairmq/tools/RateLimit.h>
#include <fairmq/tools/Network.h>

#include <boost/algorithm/string.hpp> // join/split

#include <list>
#include <chrono>
#include <mutex>
#include <thread>
#include <iomanip>
#include <future>
#include <algorithm> // std::max

using namespace std;
using namespace fair::mq;

static map<Transition, State> backwardsCompatibilityWaitForEndOfStateHelper =
{
    { Transition::InitDevice,   State::InitializingDevice },
    { Transition::CompleteInit, State::Initialized },
    { Transition::Bind,         State::Bound },
    { Transition::Connect,      State::DeviceReady },
    { Transition::InitTask,     State::Ready },
    { Transition::Run,          State::Ready },
    { Transition::Stop,         State::Ready },
    { Transition::ResetTask,    State::DeviceReady },
    { Transition::ResetDevice,  State::Idle }
};

static map<int, Transition> backwardsCompatibilityChangeStateHelper =
{
    { FairMQDevice::Event::INIT_DEVICE,           Transition::InitDevice },
    { FairMQDevice::Event::internal_DEVICE_READY, Transition::Auto },
    { FairMQDevice::Event::INIT_TASK,             Transition::InitTask },
    { FairMQDevice::Event::internal_READY,        Transition::Auto },
    { FairMQDevice::Event::RUN,                   Transition::Run },
    { FairMQDevice::Event::STOP,                  Transition::Stop },
    { FairMQDevice::Event::RESET_TASK,            Transition::ResetTask },
    { FairMQDevice::Event::RESET_DEVICE,          Transition::ResetDevice },
    { FairMQDevice::Event::internal_IDLE,         Transition::Auto },
    { FairMQDevice::Event::END,                   Transition::End },
    { FairMQDevice::Event::ERROR_FOUND,           Transition::ErrorFound }
};

constexpr const char* FairMQDevice::DefaultId;
constexpr int FairMQDevice::DefaultIOThreads;
constexpr const char* FairMQDevice::DefaultTransportName;
constexpr fair::mq::Transport FairMQDevice::DefaultTransportType;
constexpr const char* FairMQDevice::DefaultNetworkInterface;
constexpr int FairMQDevice::DefaultInitTimeout;
constexpr uint64_t FairMQDevice::DefaultMaxRunTime;
constexpr float FairMQDevice::DefaultRate;
constexpr const char* FairMQDevice::DefaultSession;

struct StateSubscription
{
    fair::mq::StateMachine& fStateMachine;
    fair::mq::StateQueue& fStateQueue;
    string fId;

    explicit StateSubscription(const string& id, fair::mq::StateMachine& stateMachine, fair::mq::StateQueue& stateQueue)
        : fStateMachine(stateMachine)
        , fStateQueue(stateQueue)
        , fId(id)
    {
        fStateMachine.SubscribeToStateChange(fId, [&](fair::mq::State state) {
            fStateQueue.Push(state);
        });
    }

    ~StateSubscription() {
        fStateMachine.UnsubscribeFromStateChange(fId);
    }
};

FairMQDevice::FairMQDevice()
    : FairMQDevice(nullptr, {0, 0, 0})
{}

FairMQDevice::FairMQDevice(ProgOptions& config)
    : FairMQDevice(&config, {0, 0, 0})
{}

FairMQDevice::FairMQDevice(const tools::Version version)
    : FairMQDevice(nullptr, version)
{}

FairMQDevice::FairMQDevice(ProgOptions& config, const tools::Version version)
    : FairMQDevice(&config, version)
{}

FairMQDevice::FairMQDevice(ProgOptions* config, const tools::Version version)
    : fTransportFactory(nullptr)
    , fTransports()
    , fChannels()
    , fInternalConfig(config ? nullptr : tools::make_unique<ProgOptions>())
    , fConfig(config ? config : fInternalConfig.get())
    , fId(DefaultId)
    , fDefaultTransportType(DefaultTransportType)
    , fStateMachine()
    , fUninitializedBindingChannels()
    , fUninitializedConnectingChannels()
    , fDataCallbacks(false)
    , fMsgInputs()
    , fMultipartInputs()
    , fMultitransportInputs()
    , fChannelRegistry()
    , fInputChannelKeys()
    , fMultitransportMutex()
    , fMultitransportProceed(false)
    , fVersion(version)
    , fRate(DefaultRate)
    , fMaxRunRuntimeInS(DefaultMaxRunTime)
    , fInitializationTimeoutInS(DefaultInitTimeout)
    , fRawCmdLineArgs()
    , fTransitionMtx()
    , fTransitioning(false)
{
    SubscribeToNewTransition("device", [&](Transition transition) {
        LOG(trace) << "device notified on new transition: " << transition;

        switch (transition) {
            case Transition::Stop:
                UnblockTransports();
                break;
            default:
                break;
        }
    });

    fStateMachine.HandleStates([&](fair::mq::State state) {
        LOG(trace) << "device notified on new state: " << state;

        fStateQueue.Push(state);

        switch (state) {
            case fair::mq::State::InitializingDevice:
                InitWrapper();
                break;
            case fair::mq::State::Binding:
                BindWrapper();
                break;
            case fair::mq::State::Connecting:
                ConnectWrapper();
                break;
            case fair::mq::State::InitializingTask:
                InitTaskWrapper();
                break;
            case fair::mq::State::Running:
                RunWrapper();
                break;
            case fair::mq::State::ResettingTask:
                ResetTaskWrapper();
                break;
            case fair::mq::State::ResettingDevice:
                ResetWrapper();
                break;
            case fair::mq::State::Exiting:
                Exit();
                break;
            default:
                LOG(trace) << "device notified on new state without a matching handler: " << state;
                break;
        }
    });

    fStateMachine.Start();
}

void FairMQDevice::TransitionTo(const fair::mq::State s)
{
    {
        lock_guard<mutex> lock(fTransitionMtx);
        if (fTransitioning) {
            LOG(debug) << "Attempting a transition with TransitionTo() while another one is already in progress";
            throw OngoingTransition("Attempting a transition with TransitionTo() while another one is already in progress");
        }
        fTransitioning = true;
    }

    using fair::mq::State;

    StateQueue sq;
    StateSubscription ss(tools::ToString(fId, ".TransitionTo"), fStateMachine, sq);

    State currentState = GetCurrentState();

    while (s != currentState) {
        switch (currentState) {
            case State::Idle:
                if (s == State::Exiting) { ChangeState(Transition::End); }
                else { ChangeState(Transition::InitDevice); }
            break;
            case State::InitializingDevice:
                ChangeState(Transition::CompleteInit);
            break;
            case State::Initialized:
                if (s == State::Exiting || s == State::Idle) { ChangeState(Transition::ResetDevice); }
                else { ChangeState(Transition::Bind); }
            break;
            case State::Bound:
                if (s == State::DeviceReady || s == State::Ready || s == State::Running) { ChangeState(Transition::Connect); }
                else { ChangeState(Transition::ResetDevice); }
            break;
            case State::DeviceReady:
                if (s == State::Running || s == State::Ready) { ChangeState(Transition::InitTask); }
                else { ChangeState(Transition::ResetDevice); }
            break;
            case State::Ready:
                if (s == State::Running) { ChangeState(Transition::Run); }
                else { ChangeState(Transition::ResetTask); }
            break;
            case State::Running:
                ChangeState(Transition::Stop);
            break;
            case State::Binding:
            case State::Connecting:
            case State::InitializingTask:
            case State::ResettingDevice:
            case State::ResettingTask:
                LOG(debug) << "TransitionTo ignoring state: " << currentState << " (expected, automatic transition).";
            break;
            default:
                LOG(debug) << "TransitionTo ignoring state: " << currentState;
            break;
        }

        currentState = sq.WaitForNext();
    }

    {
        lock_guard<mutex> lock(fTransitionMtx);
        fTransitioning = false;
    }
}

bool FairMQDevice::ChangeState(const int transition)
{
    return ChangeState(backwardsCompatibilityChangeStateHelper.at(transition));
}

void FairMQDevice::WaitForEndOfState(Transition transition)
{
    WaitForState(backwardsCompatibilityWaitForEndOfStateHelper.at(transition));
}

void FairMQDevice::InitWrapper()
{
    // run initialization once CompleteInit transition is requested
    fStateMachine.WaitForPendingState();

    fId = fConfig->GetProperty<string>("id", DefaultId);

    Init();

    fRate = fConfig->GetProperty<float>("rate", DefaultRate);
    fMaxRunRuntimeInS = fConfig->GetProperty<uint64_t>("max-run-time", DefaultMaxRunTime);
    fInitializationTimeoutInS = fConfig->GetProperty<int>("init-timeout", DefaultInitTimeout);

    try {
        fDefaultTransportType = fair::mq::TransportTypes.at(fConfig->GetProperty<string>("transport", DefaultTransportName));
    } catch (const exception& e) {
        LOG(error) << "exception: " << e.what();
        LOG(error) << "invalid transport type provided: " << fConfig->GetProperty<string>("transport", "not provided");
        throw;
    }

    unordered_map<string, int> infos = fConfig->GetChannelInfo();
    for (const auto& info : infos) {
        for (int i = 0; i < info.second; ++i) {
            fChannels[info.first].emplace_back(info.first, i, fConfig->GetPropertiesStartingWith(tools::ToString("chans.", info.first, ".", i, ".")));
        }
    }

    LOG(debug) << "Setting '" << fair::mq::TransportNames.at(fDefaultTransportType) << "' as default transport for the device";
    fTransportFactory = AddTransport(fDefaultTransportType);

    string networkInterface = fConfig->GetProperty<string>("network-interface", DefaultNetworkInterface);

    // Fill the uninitialized channel containers
    for (auto& channel : fChannels) {
        int subChannelIndex = 0;
        for (auto& subChannel : channel.second) {
            // set channel transport
            LOG(debug) << "Initializing transport for channel " << subChannel.fName << ": " << fair::mq::TransportNames.at(subChannel.fTransportType);
            subChannel.InitTransport(AddTransport(subChannel.fTransportType));

            if (subChannel.fMethod == "bind") {
                // if binding address is not specified, try getting it from the configured network interface
                if (subChannel.fAddress == "unspecified" || subChannel.fAddress == "") {
                    // if the configured network interface is default, get its name from the default route
                    try {
                        if (networkInterface == "default") {
                            networkInterface = tools::getDefaultRouteNetworkInterface();
                        }
                        subChannel.fAddress = "tcp://" + tools::getInterfaceIP(networkInterface) + ":1";
                    } catch(const tools::DefaultRouteDetectionError& e) {
                        LOG(debug) << "binding on tcp://*:1";
                        subChannel.fAddress = "tcp://*:1";
                    }
                }
                // fill the uninitialized list
                fUninitializedBindingChannels.push_back(&subChannel);
            } else if (subChannel.fMethod == "connect") {
                // fill the uninitialized list
                fUninitializedConnectingChannels.push_back(&subChannel);
            } else if (subChannel.fAddress.find_first_of("@+>") != string::npos) {
                // fill the uninitialized list
                fUninitializedConnectingChannels.push_back(&subChannel);
            } else {
                LOG(error) << "Cannot update configuration. Socket method (bind/connect) for channel '" << subChannel.fName << "' not specified.";
                throw runtime_error(tools::ToString("Cannot update configuration. Socket method (bind/connect) for channel ", subChannel.fName, " not specified."));
            }

            subChannelIndex++;
        }
    }

    // ChangeState(Transition::Auto);
}

void FairMQDevice::BindWrapper()
{
    // Bind channels. Here one run is enough, because bind settings should be available locally
    // If necessary this could be handled in the same way as the connecting channels
    AttachChannels(fUninitializedBindingChannels);

    if (!fUninitializedBindingChannels.empty()) {
        LOG(error) << fUninitializedBindingChannels.size() << " of the binding channels could not initialize. Initial configuration incomplete.";
        throw runtime_error(tools::ToString(fUninitializedBindingChannels.size(), " of the binding channels could not initialize. Initial configuration incomplete."));
    }

    Bind();

    ChangeState(Transition::Auto);
}

void FairMQDevice::ConnectWrapper()
{
    // go over the list of channels until all are initialized (and removed from the uninitialized list)
    int numAttempts = 1;
    auto sleepTimeInMS = 50;
    auto maxAttempts = fInitializationTimeoutInS * 1000 / sleepTimeInMS;
    // first attempt
    AttachChannels(fUninitializedConnectingChannels);
    // if not all channels could be connected, update their address values from config and retry
    while (!fUninitializedConnectingChannels.empty()) {
        this_thread::sleep_for(chrono::milliseconds(sleepTimeInMS));

        for (auto& chan : fUninitializedConnectingChannels) {
            string key{"chans." + chan->GetPrefix() + "." + chan->GetIndex() + ".address"};
            string newAddress = fConfig->GetProperty<string>(key);
            if (newAddress != chan->GetAddress()) {
                chan->UpdateAddress(newAddress);
            }
        }

        if (numAttempts++ > maxAttempts) {
            LOG(error) << "could not connect all channels after " << fInitializationTimeoutInS << " attempts";
            throw runtime_error(tools::ToString("could not connect all channels after ", fInitializationTimeoutInS, " attempts"));
        }

        AttachChannels(fUninitializedConnectingChannels);
    }

    if (fChannels.empty()) {
        LOG(warn) << "No channels created after finishing initialization";
    }

    Connect();

    ChangeState(Transition::Auto);
}

void FairMQDevice::AttachChannels(vector<FairMQChannel*>& chans)
{
    auto itr = chans.begin();

    while (itr != chans.end()) {
        if ((*itr)->Validate()) {
            (*itr)->Init();
            if (AttachChannel(**itr)) {
                (*itr)->SetModified(false);
                // remove the channel from the uninitialized container
                itr = chans.erase(itr);
            } else {
                LOG(error) << "failed to attach channel " << (*itr)->fName << " (" << (*itr)->fMethod << ")";
                ++itr;
            }
        } else {
            ++itr;
        }
    }
}

bool FairMQDevice::AttachChannel(FairMQChannel& chan)
{
    vector<string> endpoints;
    string chanAddress = chan.GetAddress();
    boost::algorithm::split(endpoints, chanAddress, boost::algorithm::is_any_of(","));

    for (auto& endpoint : endpoints) {
        // attach
        bool bind = (chan.GetMethod() == "bind");
        bool connectionModifier = false;
        string address = endpoint;

        // check if the default fMethod is overridden by a modifier
        if (endpoint[0] == '+' || endpoint[0] == '>') {
            connectionModifier = true;
            bind = false;
            address = endpoint.substr(1);
        } else if (endpoint[0] == '@') {
            connectionModifier = true;
            bind = true;
            address = endpoint.substr(1);
        }

        if (address.compare(0, 6, "tcp://") == 0) {
            string addressString = address.substr(6);
            auto pos = addressString.find(':');
            string hostPart = addressString.substr(0, pos);
            if (!(bind && hostPart == "*")) {
                string portPart = addressString.substr(pos + 1);
                string resolvedHost = tools::getIpFromHostname(hostPart);
                if (resolvedHost == "") {
                    return false;
                }
                address.assign("tcp://" + resolvedHost + ":" + portPart);
            }
        }

        bool success = true;
        // make the connection
        if (bind) {
            success = chan.BindEndpoint(address);
        } else {
            success = chan.ConnectEndpoint(address);
        }

        // bind might bind to an address different than requested,
        // put the actual address back in the config
        endpoint.clear();
        if (connectionModifier) {
            endpoint.push_back(bind?'@':'+');
        }
        endpoint += address;

        // after the book keeping is done, exit in case of errors
        if (!success) {
            return success;
        } else {
            LOG(debug) << "Attached channel " << chan.GetName() << " to " << endpoint << (bind ? " (bind) " : " (connect) ") << "(" << chan.GetType() << ")";
        }
    }

    // put the (possibly) modified address back in the channel object and config
    string newAddress(boost::algorithm::join(endpoints, ","));
    if (newAddress != chanAddress) {
        chan.UpdateAddress(newAddress);

        // update address in the config, it could have been modified during binding
        fConfig->SetProperty({"chans." + chan.GetPrefix() + "." + chan.GetIndex() + ".address"}, newAddress);
    }

    return true;
}

void FairMQDevice::InitTaskWrapper()
{
    InitTask();

    ChangeState(Transition::Auto);
}

void FairMQDevice::RunWrapper()
{
    LOG(info) << "DEVICE: Running...";

    // start the rate logger thread
    future<void> rateLogger = async(launch::async, &FairMQDevice::LogSocketRates, this);

    // notify transports to resume transfers
    for (auto& t : fTransports) {
        t.second->Resume();
    }

    try {
        PreRun();

        // process either data callbacks or ConditionalRun/Run
        if (fDataCallbacks) {
            // if only one input channel, do lightweight handling without additional polling.
            if (fInputChannelKeys.size() == 1 && fChannels.at(fInputChannelKeys.at(0)).size() == 1) {
                HandleSingleChannelInput();
            } else {// otherwise do full handling with polling
                HandleMultipleChannelInput();
            }
        } else {
            tools::RateLimiter rateLimiter(fRate);

            while (!NewStatePending() && ConditionalRun()) {
                if (fRate > 0.001) {
                    rateLimiter.maybe_sleep();
                }
            }

            Run();
        }

        // if Run() exited and the state is still RUNNING, transition to READY.
        if (!NewStatePending()) {
            UnblockTransports();
            ChangeState(Transition::Stop);
        }

        PostRun();

    } catch (const out_of_range& oor) {
        LOG(error) << "out of range: " << oor.what();
        LOG(error) << "incorrect/incomplete channel configuration?";
        ChangeState(Transition::ErrorFound);
        throw;
    } catch (...) {
        ChangeState(Transition::ErrorFound);
        throw;
    }

    rateLogger.get();
}

void FairMQDevice::HandleSingleChannelInput()
{
    bool proceed = true;

    if (fMsgInputs.size() > 0)
    {
        while (!NewStatePending() && proceed)
        {
            proceed = HandleMsgInput(fInputChannelKeys.at(0), fMsgInputs.begin()->second, 0);
        }
    }
    else if (fMultipartInputs.size() > 0)
    {
        while (!NewStatePending() && proceed)
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
        fair::mq::Transport t = fChannels.at(k).at(0).fTransportType;
        if (fMultitransportInputs.find(t) == fMultitransportInputs.end())
        {
            fMultitransportInputs.insert(pair<fair::mq::Transport, vector<string>>(t, vector<string>()));
            fMultitransportInputs.at(t).push_back(k);
        }
        else
        {
            fMultitransportInputs.at(t).push_back(k);
        }
    }

    for (const auto& mi : fMsgInputs)
    {
        for (auto& i : fChannels.at(mi.first))
        {
            i.fMultipart = false;
        }
    }

    for (const auto& mi : fMultipartInputs)
    {
        for (auto& i : fChannels.at(mi.first))
        {
            i.fMultipart = true;
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

        while (!NewStatePending() && proceed)
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
        threads.emplace_back(thread(&FairMQDevice::PollForTransport, this, fTransports.at(i.first).get(), i.second));
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

        while (!NewStatePending() && fMultitransportProceed)
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
        throw runtime_error(tools::ToString("FairMQDevice::PollForTransport() failed: ", e.what(), ", going to ERROR state."));
    }
}

bool FairMQDevice::HandleMsgInput(const string& chName, const InputMsgCallback& callback, int i)
{
    unique_ptr<FairMQMessage> input(fChannels.at(chName).at(i).fTransportFactory->CreateMessage());

    if (Receive(input, chName, i) >= 0)
    {
        return callback(input, i);
    }
    else
    {
        return false;
    }
}

bool FairMQDevice::HandleMultipartInput(const string& chName, const InputMultipartCallback& callback, int i)
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

shared_ptr<FairMQTransportFactory> FairMQDevice::AddTransport(fair::mq::Transport transport)
{
    if (transport == fair::mq::Transport::DEFAULT) {
        transport = fDefaultTransportType;
    }

    auto i = fTransports.find(transport);

    if (i == fTransports.end()) {
        LOG(debug) << "Adding '" << fair::mq::TransportNames.at(transport) << "' transport";
        auto tr = FairMQTransportFactory::CreateTransportFactory(fair::mq::TransportNames.at(transport), fId, fConfig);
        fTransports.insert({transport, tr});
        return tr;
    } else {
        LOG(debug) << "Reusing existing '" << fair::mq::TransportNames.at(transport) << "' transport";
        return i->second;
    }
}

void FairMQDevice::SetConfig(ProgOptions& config)
{
    fInternalConfig.reset();
    fConfig = &config;
}

void FairMQDevice::LogSocketRates()
{
    vector<FairMQChannel*> filteredChannels;
    vector<string> filteredChannelNames;
    vector<int> logIntervals;
    vector<int> intervalCounters;

    size_t chanNameLen = 0;

    // iterate over the channels map
    for (auto& channel : fChannels) {
        // iterate over the channels vector
        for (auto& subChannel : channel.second) {
            if (subChannel.fRateLogging > 0) {
                filteredChannels.push_back(&subChannel);
                logIntervals.push_back(subChannel.fRateLogging);
                intervalCounters.push_back(0);
                filteredChannelNames.push_back(subChannel.GetName());
                chanNameLen = max(chanNameLen, filteredChannelNames.back().length());
            }
        }
    }

    vector<unsigned long> bytesIn(filteredChannels.size());
    vector<unsigned long> msgIn(filteredChannels.size());
    vector<unsigned long> bytesOut(filteredChannels.size());
    vector<unsigned long> msgOut(filteredChannels.size());

    vector<unsigned long> bytesInNew(filteredChannels.size());
    vector<unsigned long> msgInNew(filteredChannels.size());
    vector<unsigned long> bytesOutNew(filteredChannels.size());
    vector<unsigned long> msgOutNew(filteredChannels.size());

    vector<double> mbPerSecIn(filteredChannels.size());
    vector<double> msgPerSecIn(filteredChannels.size());
    vector<double> mbPerSecOut(filteredChannels.size());
    vector<double> msgPerSecOut(filteredChannels.size());

    int i = 0;
    for (const auto& channel : filteredChannels) {
        bytesIn.at(i) = channel->GetBytesRx();
        bytesOut.at(i) = channel->GetBytesTx();
        msgIn.at(i) = channel->GetMessagesRx();
        msgOut.at(i) = channel->GetMessagesTx();
        ++i;
    }

    chrono::time_point<chrono::high_resolution_clock> t0(chrono::high_resolution_clock::now());
    chrono::time_point<chrono::high_resolution_clock> t1;
    uint64_t secondsElapsed = 0;

    while (!NewStatePending()) {
        WaitFor(chrono::seconds(1));

        t1 = chrono::high_resolution_clock::now();

        uint64_t msSinceLastLog = chrono::duration_cast<chrono::milliseconds>(t1 - t0).count();

        i = 0;

        for (const auto& channel : filteredChannels) {
            intervalCounters.at(i)++;

            if (intervalCounters.at(i) == logIntervals.at(i)) {
                intervalCounters.at(i) = 0;

                if (msSinceLastLog > 0) {
                    bytesInNew.at(i) = channel->GetBytesRx();
                    msgInNew.at(i) = channel->GetMessagesRx();
                    bytesOutNew.at(i) = channel->GetBytesTx();
                    msgOutNew.at(i) = channel->GetMessagesTx();

                    mbPerSecIn.at(i) = (static_cast<double>(bytesInNew.at(i) - bytesIn.at(i)) / (1000. * 1000.)) / static_cast<double>(msSinceLastLog) * 1000.;
                    msgPerSecIn.at(i) = static_cast<double>(msgInNew.at(i) - msgIn.at(i)) / static_cast<double>(msSinceLastLog) * 1000.;
                    mbPerSecOut.at(i) = (static_cast<double>(bytesOutNew.at(i) - bytesOut.at(i)) / (1000. * 1000.)) / static_cast<double>(msSinceLastLog) * 1000.;
                    msgPerSecOut.at(i) = static_cast<double>(msgOutNew.at(i) - msgOut.at(i)) / static_cast<double>(msSinceLastLog) * 1000.;

                    bytesIn.at(i) = bytesInNew.at(i);
                    msgIn.at(i) = msgInNew.at(i);
                    bytesOut.at(i) = bytesOutNew.at(i);
                    msgOut.at(i) = msgOutNew.at(i);

                    LOG(info) << setw(chanNameLen) << filteredChannelNames.at(i) << ": "
                              << "in: " << msgPerSecIn.at(i) << " (" << mbPerSecIn.at(i) << " MB) "
                              << "out: " << msgPerSecOut.at(i) << " (" << mbPerSecOut.at(i) << " MB)";
                }
            }

            ++i;
        }

        t0 = t1;
        if (fMaxRunRuntimeInS > 0 && ++secondsElapsed >= fMaxRunRuntimeInS) {
            ChangeState(Transition::Stop);
        }
    }
}

void FairMQDevice::UnblockTransports()
{
    for (auto& transport : fTransports) {
        transport.second->Interrupt();
    }
}

void FairMQDevice::ResetTaskWrapper()
{
    ResetTask();

    ChangeState(Transition::Auto);
}

void FairMQDevice::ResetWrapper()
{
    for (auto& transport : fTransports) {
        transport.second->Reset();
    }

    Reset();

    fChannels.clear();
    fTransports.clear();
    fTransportFactory.reset();
    ChangeState(Transition::Auto);
}

FairMQDevice::~FairMQDevice()
{
    UnsubscribeFromNewTransition("device");
    fStateMachine.StopHandlingStates();
    LOG(debug) << "Shutting down device " << fId;
}
