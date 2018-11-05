/********************************************************************************
 * Copyright (C) 2012-2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <FairMQDevice.h>

#include <boost/algorithm/string.hpp> // join/split

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <list>
#include <cstdlib>
#include <chrono>
#include <mutex>
#include <thread>
#include <functional>
#include <sstream>
#include <iomanip>
#include <future>
#include <algorithm> // std::max

using namespace std;

FairMQDevice::FairMQDevice()
    : FairMQDevice(nullptr, {0, 0, 0})
{
}

FairMQDevice::FairMQDevice(FairMQProgOptions& config)
    : FairMQDevice(&config, {0, 0, 0})
{
}

FairMQDevice::FairMQDevice(const fair::mq::tools::Version version)
    : FairMQDevice(nullptr, version)
{
}

FairMQDevice::FairMQDevice(FairMQProgOptions& config, const fair::mq::tools::Version version)
    : FairMQDevice(&config, version)
{
}

FairMQDevice::FairMQDevice(FairMQProgOptions* config, const fair::mq::tools::Version version)
    : fTransportFactory(nullptr)
    , fTransports()
    , fChannels()
    , fInternalConfig(config ? nullptr : fair::mq::tools::make_unique<FairMQProgOptions>())
    , fConfig(config ? config : fInternalConfig.get())
    , fId()
    , fDefaultTransportType(fair::mq::Transport::DEFAULT)
    , fDataCallbacks(false)
    , fMsgInputs()
    , fMultipartInputs()
    , fMultitransportInputs()
    , fChannelRegistry()
    , fInputChannelKeys()
    , fMultitransportMutex()
    , fMultitransportProceed(false)
    , fVersion(version)
    , fRate(0.)
    , fRawCmdLineArgs()
    , fInterrupted(false)
    , fInterruptedCV()
    , fInterruptedMtx()
    , fRateLogging(true)
{
}

void FairMQDevice::InitWrapper()
{
    fId = fConfig->GetValue<string>("id");
    fRate = fConfig->GetValue<float>("rate");

    try {
        fDefaultTransportType = fair::mq::TransportTypes.at(fConfig->GetValue<string>("transport"));
    } catch (const exception& e) {
        LOG(error) << "exception: " << e.what();
        LOG(error) << "invalid transport type provided: " << fConfig->GetValue<string>("transport");
        throw;
    }

    for (auto& c : fConfig->GetFairMQMap()) {
        if (fChannels.find(c.first) == fChannels.end()) {
            LOG(debug) << "Inserting new device channel from config: " << c.first;
            fChannels.insert(c);
        } else {
            LOG(debug) << "Updating existing device channel from config: " << c.first;
            fChannels[c.first] = c.second;
        }
    }

    LOG(debug) << "Requesting '" << fair::mq::TransportNames.at(fDefaultTransportType) << "' as default transport for the device";
    fTransportFactory = AddTransport(fDefaultTransportType);

    // Containers to store the uninitialized channels.
    vector<FairMQChannel*> uninitializedBindingChannels;
    vector<FairMQChannel*> uninitializedConnectingChannels;

    string networkInterface = fConfig->GetValue<string>("network-interface");

    // Fill the uninitialized channel containers
    for (auto& mi : fChannels) {
        int subChannelIndex = 0;
        for (auto& vi : mi.second) {
            // set channel name: name + vector index
            vi.fName = fair::mq::tools::ToString(mi.first, "[", subChannelIndex, "]");

            // set channel transport
            if (vi.fTransportType == fair::mq::Transport::DEFAULT || vi.fTransportType == fTransportFactory->GetType()) {
                LOG(debug) << vi.fName << ": using default transport";
                vi.InitTransport(fTransportFactory);
            } else {
                LOG(debug) << vi.fName << ": channel transport (" << fair::mq::TransportNames.at(fDefaultTransportType) << ") overriden to " << fair::mq::TransportNames.at(vi.fTransportType);
                vi.InitTransport(AddTransport(vi.fTransportType));
            }

            if (vi.fMethod == "bind") {
                // if binding address is not specified, try getting it from the configured network interface
                if (vi.fAddress == "unspecified" || vi.fAddress == "") {
                    // if the configured network interface is default, get its name from the default route
                    if (networkInterface == "default") {
                        networkInterface = fair::mq::tools::getDefaultRouteNetworkInterface();
                    }
                    vi.fAddress = "tcp://" + fair::mq::tools::getInterfaceIP(networkInterface) + ":1";
                }
                // fill the uninitialized list
                uninitializedBindingChannels.push_back(&vi);
            } else if (vi.fMethod == "connect") {
                // fill the uninitialized list
                uninitializedConnectingChannels.push_back(&vi);
            } else if (vi.fAddress.find_first_of("@+>") != string::npos) {
                // fill the uninitialized list
                uninitializedConnectingChannels.push_back(&vi);
            } else {
                LOG(error) << "Cannot update configuration. Socket method (bind/connect) for channel '" << vi.fName << "' not specified.";
                throw runtime_error(fair::mq::tools::ToString("Cannot update configuration. Socket method (bind/connect) for channel ", vi.fName, " not specified."));
            }

            subChannelIndex++;
        }
    }

    // Bind channels. Here one run is enough, because bind settings should be available locally
    // If necessary this could be handled in the same way as the connecting channels
    AttachChannels(uninitializedBindingChannels);

    if (!uninitializedBindingChannels.empty()) {
        LOG(error) << uninitializedBindingChannels.size() << " of the binding channels could not initialize. Initial configuration incomplete.";
        throw runtime_error(fair::mq::tools::ToString(uninitializedBindingChannels.size(), " of the binding channels could not initialize. Initial configuration incomplete."));
    }

    CallStateChangeCallbacks(INITIALIZING_DEVICE);

    int initializationTimeoutInS = fConfig->GetValue<int>("initialization-timeout");

    // go over the list of channels until all are initialized (and removed from the uninitialized list)
    int numAttempts = 1;
    auto sleepTimeInMS = 50;
    auto maxAttempts = initializationTimeoutInS * 1000 / sleepTimeInMS;
    // first attempt
    AttachChannels(uninitializedConnectingChannels);
    // if not all channels could be connected, update their address values from config and retry
    while (!uninitializedConnectingChannels.empty()) {
        this_thread::sleep_for(chrono::milliseconds(sleepTimeInMS));

        for (auto& chan : uninitializedConnectingChannels) {
            string key{"chans." + chan->GetChannelPrefix() + "." + chan->GetChannelIndex() + ".address"};
            string newAddress = fConfig->GetValue<string>(key);
            if (newAddress != chan->GetAddress()) {
                chan->UpdateAddress(newAddress);
            }
        }

        if (numAttempts++ > maxAttempts) {
            LOG(error) << "could not connect all channels after " << initializationTimeoutInS << " attempts";
            throw runtime_error(fair::mq::tools::ToString("could not connect all channels after ", initializationTimeoutInS, " attempts"));
        }

        AttachChannels(uninitializedConnectingChannels);
    }

    Init();

    if (fChannels.empty()) {
        LOG(warn) << "No channels created after finishing initialization";
    }

    ChangeState(internal_DEVICE_READY);
}

void FairMQDevice::Init()
{
}

void FairMQDevice::AttachChannels(vector<FairMQChannel*>& chans)
{
    auto itr = chans.begin();

    while (itr != chans.end()) {
        if ((*itr)->ValidateChannel()) {
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
                string resolvedHost = fair::mq::tools::getIpFromHostname(hostPart);
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
        fConfig->SetValue({"chans." + chan.GetPrefix() + "." + chan.GetIndex() + ".address"}, newAddress);
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

void FairMQDevice::RunWrapper()
{
    CallStateChangeCallbacks(RUNNING);

    LOG(info) << "DEVICE: Running...";

    // start the rate logger thread
    fRateLogging = true;
    future<void> rateLogger = async(launch::async, &FairMQDevice::LogSocketRates, this);

    // notify transports to resume transfers
    {
        lock_guard<mutex> guard(fInterruptedMtx);
        fInterrupted = false;
    }
    for (auto& t : fTransports)
    {
        t.second->Resume();
    }

    try {
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
            fair::mq::tools::RateLimiter rateLimiter(fRate);

            while (CheckCurrentState(RUNNING) && ConditionalRun())
            {
                if (fRate > 0.001)
                {
                    rateLimiter.maybe_sleep();
                }
            }

            Run();
        }
    } catch (const out_of_range& oor) {
        LOG(error) << "out of range: " << oor.what();
        LOG(error) << "incorrect/incomplete channel configuration?";
        fRateLogging = false;
        throw;
    } catch (...) {
        fRateLogging = false;
        throw;
    }

    // if Run() exited and the state is still RUNNING, transition to READY.
    if (CheckCurrentState(RUNNING))
    {
        ChangeState(internal_READY);
    }

    PostRun();

    rateLogger.get();
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
        throw runtime_error(fair::mq::tools::ToString("FairMQDevice::PollForTransport() failed: ", e.what(), ", going to ERROR state."));
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

shared_ptr<FairMQTransportFactory> FairMQDevice::AddTransport(const fair::mq::Transport transport)
{
    auto i = fTransports.find(transport);

    if (i == fTransports.end())
    {
        auto tr = FairMQTransportFactory::CreateTransportFactory(fair::mq::TransportNames.at(transport), fId, fConfig);

        LOG(debug) << "Adding '" << fair::mq::TransportNames.at(transport) << "' transport to the device.";

        pair<fair::mq::Transport, shared_ptr<FairMQTransportFactory>> trPair(transport, tr);
        fTransports.insert(trPair);

        return tr;
    }
    else
    {
        LOG(debug) << "Reusing existing '" << fair::mq::TransportNames.at(transport) << "' transport.";
        return i->second;
    }
}

void FairMQDevice::SetConfig(FairMQProgOptions& config)
{
    fInternalConfig.reset();
    fConfig = &config;
}

void FairMQDevice::LogSocketRates()
{
    chrono::time_point<chrono::high_resolution_clock> t0;
    chrono::time_point<chrono::high_resolution_clock> t1;

    vector<FairMQSocket*> filteredSockets;
    vector<string> filteredChannelNames;
    vector<int> logIntervals;
    vector<int> intervalCounters;

    size_t chanNameLen = 0;

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
                chanNameLen = max(chanNameLen, filteredChannelNames.back().length());
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

        while (fRateLogging)
        {
            t1 = chrono::high_resolution_clock::now();

            unsigned long long msSinceLastLog = chrono::duration_cast<chrono::milliseconds>(t1 - t0).count();

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

                    LOG(info) << setw(chanNameLen) << filteredChannelNames.at(i) << ": "
                              << "in: " << msgPerSecIn.at(i) << " (" << mbPerSecIn.at(i) << " MB) "
                              << "out: " << msgPerSecOut.at(i) << " (" << mbPerSecOut.at(i) << " MB)";
                }

                ++i;
            }

            t0 = t1;
            WaitFor(chrono::milliseconds(1000));
        }
    }
}

void FairMQDevice::Unblock()
{
    for (auto& t : fTransports)
    {
        t.second->Interrupt();
    }
    {
        lock_guard<mutex> guard(fInterruptedMtx);
        fInterrupted = true;
        fRateLogging = false;
    }
    fInterruptedCV.notify_all();
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

    for (auto& t : fTransports)
    {
        t.second->Reset();
    }

    // iterate over the channels map
    for (auto& mi : fChannels)
    {
        // iterate over the channels vector
        for (auto& vi : mi.second)
        {
            // vi.fReset = true;
            vi.fSocket.reset(); // destroy FairMQSocket
        }
    }

    Reset();

    ChangeState(internal_IDLE);
}

void FairMQDevice::Reset()
{
}

void FairMQDevice::Exit()
{
}

FairMQDevice::~FairMQDevice()
{
    LOG(debug) << "Destructing device " << fId;
}
