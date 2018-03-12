/********************************************************************************
 *    Copyright (C) 2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "DDS.h"

#include <fairmq/Tools.h>

#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/split.hpp>

#include <termios.h> // for the interactive mode
#include <poll.h> // for the interactive mode
#include <sstream>
#include <iostream>
#include <cstdlib>
#include <stdexcept>

using namespace std;
using fair::mq::tools::ToString;

namespace fair
{
namespace mq
{
namespace plugins
{

DDS::DDS(const string name, const Plugin::Version version, const string maintainer, const string homepage, PluginServices* pluginServices)
    : Plugin(name, version, maintainer, homepage, pluginServices)
    , fService()
    , fDDSCustomCmd(fService)
    , fDDSKeyValue(fService)
    , fBindingChans()
    , fConnectingChans()
    , fStopMutex()
    , fStopCondition()
    , fCommands({ "INIT DEVICE", "INIT TASK", "PAUSE", "RUN", "STOP", "RESET TASK", "RESET DEVICE" })
    , fControllerThread()
    , fEvents()
    , fEventsMutex()
    , fNewEvent()
    , fDeviceTerminationRequested(false)
    , fHeartbeatInterval{100}
{
    try
    {
        TakeDeviceControl();
        fControllerThread = thread(&DDS::HandleControl, this);
        fHeartbeatThread = thread(&DDS::HeartbeatSender, this);
    }
    catch (PluginServices::DeviceControlError& e)
    {
        LOG(debug) << e.what();
    }
    catch (exception& e)
    {
        LOG(error) << "Error in plugin initialization: " << e.what();
    }
}

auto DDS::HandleControl() -> void
{
    try
    {
        // subscribe for state changes from DDS (subscriptions start firing after fService.start() is called)
        SubscribeForCustomCommands();

        // subscribe for DDS service errors.
        fService.subscribeOnError([](const dds::intercom_api::EErrorCode errorCode, const string& errorMsg) {
            LOG(error) << "DDS Error received: error code: " << errorCode << ", error message: " << errorMsg << endl;
        });

        LOG(debug) << "Subscribing for DDS properties.";
        SubscribeForConnectingChannels();

        // subscribe to device state changes, pushing new state chenges into the event queue
        SubscribeToDeviceStateChange([&](DeviceState newState)
        {
            {
                lock_guard<mutex> lock{fEventsMutex};
                fEvents.push(newState);
            }
            fNewEvent.notify_one();
            if (newState == DeviceState::Exiting)
            {
                fDeviceTerminationRequested = true;
            }

            {
                lock_guard<mutex> lock{fStateChangeSubscriberMutex};
                string id = GetProperty<string>("id");
                for (auto subscriberId : fStateChangeSubscribers)
                {
                    LOG(debug) << "Publishing state-change: " << newState << " to " << subscriberId;
                    fDDSCustomCmd.send("state-change: " + id + "," + ToStr(newState), to_string(subscriberId));
                }
            }
        });

        ChangeDeviceState(DeviceStateTransition::InitDevice);
        while (WaitForNextState() != DeviceState::InitializingDevice) {}

        // in the Initializing state subscribe to receive addresses of connecting channels from DDS
        // and propagate addresses of bound channels to DDS.
        FillChannelContainers();

        LOG(DEBUG) << "$DDS_TASK_PATH: " << getenv("DDS_TASK_PATH");
        LOG(DEBUG) << "$DDS_GROUP_NAME: " << getenv("DDS_GROUP_NAME");
        LOG(DEBUG) << "$DDS_COLLECTION_NAME: " << getenv("DDS_COLLECTION_NAME");
        LOG(DEBUG) << "$DDS_TASK_NAME: " << getenv("DDS_TASK_NAME");
        LOG(DEBUG) << "$DDS_TASK_INDEX: " << getenv("DDS_TASK_INDEX");
        LOG(DEBUG) << "$DDS_COLLECTION_INDEX: " << getenv("DDS_COLLECTION_INDEX");

        // start DDS service - subscriptions will only start firing after this step
        fService.start();

        // publish bound addresses via DDS at keys corresponding to the channel prefixes, e.g. 'data' in data[i]
        PublishBoundChannels();

        while (WaitForNextState() != DeviceState::DeviceReady) {}

        ChangeDeviceState(DeviceStateTransition::InitTask);
        while (WaitForNextState() != DeviceState::Ready) {}
        ChangeDeviceState(DeviceStateTransition::Run);

        // wait until stop signal
        unique_lock<mutex> lock(fStopMutex);
        while (!fDeviceTerminationRequested)
        {
            fStopCondition.wait_for(lock, chrono::seconds(1));
        }
        LOG(debug) << "Stopping DDS control plugin";
    }
    catch (exception& e)
    {
        LOG(error) << "Error: " << e.what() << endl;
        return;
    }

    fDDSKeyValue.unsubscribe();
    fDDSCustomCmd.unsubscribe();

    try
    {
        UnsubscribeFromDeviceStateChange();
        ReleaseDeviceControl();
    }
    catch (fair::mq::PluginServices::DeviceControlError& e)
    {
        LOG(error) << e.what();
    }
}

auto DDS::FillChannelContainers() -> void
{
    try {
        unordered_map<string, int> channelInfo(GetChannelInfo());

        // fill binding and connecting chans
        for (const auto& c : channelInfo) {
            string methodKey{"chans." + c.first + "." + to_string(c.second - 1) + ".method"};
            if (GetProperty<string>(methodKey) == "bind") {
                fBindingChans.insert(make_pair(c.first, vector<string>()));
                for (int i = 0; i < c.second; ++i) {
                    fBindingChans.at(c.first).push_back(GetProperty<string>(string{"chans." + c.first + "." + to_string(i) + ".address"}));
                }
            } else if (GetProperty<string>(methodKey) == "connect") {
                fConnectingChans.insert(make_pair(c.first, DDSConfig()));
                LOG(debug) << "preparing to connect: " << c.first << " with " << c.second << " sub-channels.";
                for (int i = 0; i < c.second; ++i) {
                    fConnectingChans.at(c.first).fSubChannelAddresses.push_back(string());
                }
            } else {
                LOG(error) << "Cannot update address configuration. Channel method (bind/connect) not specified.";
                return;
            }
        }

        // save properties that will have multiple values arriving (with only some of them to be used)
        vector<string> iValues;
        if (PropertyExists("dds-i")) {
            iValues = GetProperty<vector<string>>("dds-i");
        }
        vector<string> inValues;
        if (PropertyExists("dds-i-n")) {
            inValues = GetProperty<vector<string>>("dds-i-n");
        }

        for (const auto& vi : iValues) {
            size_t pos = vi.find(":");
            string chanName = vi.substr(0, pos );

            // check if provided name is a valid channel name
            if (fConnectingChans.find(chanName) == fConnectingChans.end()) {
                throw invalid_argument(ToString("channel provided to dds-i is not an actual connecting channel of this device: ", chanName));
            }

            int i = stoi(vi.substr(pos + 1));
            LOG(debug) << "dds-i: adding " << chanName << " -> i of " << i;
            fI.insert(make_pair(chanName, i));
        }

        for (const auto& vi : inValues) {
            size_t pos = vi.find(":");
            string chanName = vi.substr(0, pos);

            // check if provided name is a valid channel name
            if (fConnectingChans.find(chanName) == fConnectingChans.end()) {
                throw invalid_argument(ToString("channel provided to dds-i-n is not an actual connecting channel of this device: ", chanName));
            }

            string i_n = vi.substr(pos + 1);
            pos = i_n.find("-");
            int i = stoi(i_n.substr(0, pos));
            int n = stoi(i_n.substr(pos + 1));
            LOG(debug) << "dds-i-n: adding " << chanName << " -> i: " << i << " n: " << n;
            fIofN.insert(make_pair(chanName, IofN(i, n)));
        }
    } catch (const exception& e) {
        LOG(error) << "Error filling channel containers: " << e.what();
    }
}

auto DDS::SubscribeForConnectingChannels() -> void
{
    fDDSKeyValue.subscribe([&] (const string& propertyId, const string& key, const string& value)
    {
        try
        {
            LOG(debug) << "Received update for " << propertyId << ": key=" << key << " value=" << value;
            string val = value;
            // check if it is to handle as one out of multiple values
            auto it = fIofN.find(propertyId);
            if (it != fIofN.end()) {
                it->second.fEntries.push_back(value);
                if (it->second.fEntries.size() == it->second.fN) {
                    sort(it->second.fEntries.begin(), it->second.fEntries.end());
                    val = it->second.fEntries.at(it->second.fI);
                } else {
                    LOG(debug) << "received " << it->second.fEntries.size() << " values for " << propertyId << ", expecting total of " << it->second.fN;
                    return;
                }
            }

            vector<string> connectionStrings;
            boost::algorithm::split(connectionStrings, val, boost::algorithm::is_any_of(","));
            if (connectionStrings.size() > 1) { // multiple bound channels received
                auto it2 = fI.find(propertyId);
                if (it2 != fI.end()) {
                    LOG(debug) << "adding connecting channel " << propertyId << " : " << connectionStrings.at(it2->second);
                    fConnectingChans.at(propertyId).fDDSValues.insert(make_pair<string, string>(key.c_str(), connectionStrings.at(it2->second).c_str()));
                } else {
                    LOG(error) << "multiple bound channels received, but no task index specified, only assigning the first";
                    fConnectingChans.at(propertyId).fDDSValues.insert(make_pair<string, string>(key.c_str(), connectionStrings.at(0).c_str()));
                }
            } else { // only one bound channel received
                fConnectingChans.at(propertyId).fDDSValues.insert(make_pair<string, string>(key.c_str(), val.c_str()));
            }

            // update channels and remove them from unfinished container
            for (auto mi = fConnectingChans.begin(); mi != fConnectingChans.end(); /* no increment */)
            {
                if (mi->second.fSubChannelAddresses.size() == mi->second.fDDSValues.size())
                {
                    // when multiple subChannels are used, their order on every device should be the same, irregardless of arrival order from DDS.
                    sort(mi->second.fSubChannelAddresses.begin(), mi->second.fSubChannelAddresses.end());
                    auto it3 = mi->second.fDDSValues.begin();
                    for (unsigned int i = 0; i < mi->second.fSubChannelAddresses.size(); ++i)
                    {
                        SetProperty<string>(string{"chans." + mi->first + "." + to_string(i) + ".address"}, it3->second);
                        ++it3;
                    }
                    fConnectingChans.erase(mi++);
                }
                else
                {
                    ++mi;
                }
            }
        }
        catch (const exception& e)
        {
            LOG(error) << "Error on handling DDS property update for " << propertyId << ": key=" << key << " value=" << value << ": " << e.what();
        }
    });
}

auto DDS::PublishBoundChannels() -> void
{
    for (const auto& chan : fBindingChans)
    {
        string joined = boost::algorithm::join(chan.second, ",");
        LOG(debug) << "Publishing " << chan.first << " bound addresses (" << chan.second.size() << ") to DDS under '" << chan.first << "' property name.";
        fDDSKeyValue.putValue(chan.first, joined);
    }
}

auto DDS::HeartbeatSender() -> void
{
    string id = GetProperty<string>("id");
    string pid(to_string(getpid()));

    while (!fDeviceTerminationRequested)
    {
        {
            lock_guard<mutex> lock{fHeartbeatSubscriberMutex};

            for (const auto subscriberId : fHeartbeatSubscribers)
            {
                fDDSCustomCmd.send("heartbeat: " + id + "," + pid, to_string(subscriberId));
            }
        }

        this_thread::sleep_for(chrono::milliseconds(fHeartbeatInterval));
    }
}

auto DDS::SubscribeForCustomCommands() -> void
{
    string id = GetProperty<string>("id");
    string pid(to_string(getpid()));

    fDDSCustomCmd.subscribe([id, pid, this](const string& cmd, const string& cond, uint64_t senderId)
    {
        LOG(info) << "Received command: " << cmd;

        if (cmd == "check-state")
        {
            fDDSCustomCmd.send(id + ": " + ToStr(GetCurrentDeviceState()) + " (pid: " + pid + ")", to_string(senderId));
        }
        else if (fCommands.find(cmd) != fCommands.end())
        {
            fDDSCustomCmd.send(id + ": attempting to " + cmd, to_string(senderId));
            ChangeDeviceState(ToDeviceStateTransition(cmd));
        }
        else if (cmd == "END")
        {
            fDDSCustomCmd.send(id + ": attempting to " + cmd, to_string(senderId));
            ChangeDeviceState(ToDeviceStateTransition(cmd));
            fDDSCustomCmd.send(id + ": " + ToStr(GetCurrentDeviceState()), to_string(senderId));
            if (ToStr(GetCurrentDeviceState()) == "EXITING")
            {
                unique_lock<mutex> lock(fStopMutex);
                fStopCondition.notify_one();
            }
        }
        else if (cmd == "dump-config")
        {
            stringstream ss;
            for (const auto pKey: GetPropertyKeys())
            {
                ss << id << ": " << pKey << " -> " << GetPropertyAsString(pKey) << endl;
            }
            fDDSCustomCmd.send(ss.str(), to_string(senderId));
        }
        else if (cmd == "subscribe-to-heartbeats")
        {
            {
                // auto size = fHeartbeatSubscribers.size();
                lock_guard<mutex> lock{fHeartbeatSubscriberMutex};
                fHeartbeatSubscribers.insert(senderId);
            }
            fDDSCustomCmd.send("heartbeat-subscription: " + id + ",OK", to_string(senderId));
        }
        else if (cmd == "unsubscribe-from-heartbeats")
        {
            {
                lock_guard<mutex> lock{fHeartbeatSubscriberMutex};
                fHeartbeatSubscribers.erase(senderId);
            }
            fDDSCustomCmd.send("heartbeat-unsubscription: " + id + ",OK", to_string(senderId));
        }
        else if (cmd == "subscribe-to-state-changes")
        {
            {
                // auto size = fStateChangeSubscribers.size();
                lock_guard<mutex> lock{fStateChangeSubscriberMutex};
                fStateChangeSubscribers.insert(senderId);
            }
            fDDSCustomCmd.send("state-changes-subscription: " + id + ",OK", to_string(senderId));
            auto state = GetCurrentDeviceState();
            LOG(debug) << "Publishing state-change: " << state << " to " << senderId;
            fDDSCustomCmd.send("state-change: " + id + "," + ToStr(state), to_string(senderId));
        }
        else if (cmd == "unsubscribe-from-state-changes")
        {
            {
                lock_guard<mutex> lock{fStateChangeSubscriberMutex};
                fStateChangeSubscribers.erase(senderId);
            }
            fDDSCustomCmd.send("state-changes-unsubscription: " + id + ",OK", to_string(senderId));
        }
        else
        {
            LOG(warn) << "Unknown command: " << cmd;
            LOG(warn) << "Origin: " << senderId;
            LOG(warn) << "Destination: " << cond;
        }
    });
}

auto DDS::WaitForNextState() -> DeviceState
{
    unique_lock<mutex> lock{fEventsMutex};
    while (fEvents.empty())
    {
        fNewEvent.wait(lock);
    }

    auto result = fEvents.front();
    fEvents.pop();
    return result;
}

DDS::~DDS()
{
    if (fControllerThread.joinable())
    {
        fControllerThread.join();
    }

    if (fHeartbeatThread.joinable())
    {
        fHeartbeatThread.join();
    }
}

} /* namespace plugins */
} /* namespace mq */
} /* namespace fair */
