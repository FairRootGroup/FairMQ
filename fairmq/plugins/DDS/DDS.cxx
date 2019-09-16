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
#include <boost/algorithm/string/classification.hpp>
#include <boost/asio/post.hpp>

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

DDS::DDS(const string& name,
         const Plugin::Version version,
         const string& maintainer,
         const string& homepage,
         PluginServices* pluginServices)
    : Plugin(name, version, maintainer, homepage, pluginServices)
    , fTransitions({"INIT DEVICE",
                    "COMPLETE INIT",
                    "BIND",
                    "CONNECT",
                    "INIT TASK",
                    "RUN",
                    "STOP",
                    "RESET TASK",
                    "RESET DEVICE",
                    "END"})
    , fCurrentState(DeviceState::Idle)
    , fLastState(DeviceState::Idle)
    , fDeviceTerminationRequested(false)
    , fLastExternalController(0)
    , fExitingAckedByLastExternalController(false)
    , fHeartbeatInterval(100)
    , fUpdatesAllowed(false)
    , fWorkGuard(fWorkerQueue.get_executor())
{
    try {
        TakeDeviceControl();

        fHeartbeatThread = thread(&DDS::HeartbeatSender, this);

        std::string deviceId(GetProperty<std::string>("id"));
        if (deviceId.empty()) {
            SetProperty<std::string>("id", dds::env_prop<dds::task_path>());
        }
        std::string sessionId(GetProperty<std::string>("session"));
        if (sessionId == "default") {
            SetProperty<std::string>("session", dds::env_prop<dds::dds_session_id>());
        }

        auto control = GetProperty<string>("control");
        bool staticMode(false);
        if (control == "static") {
            LOG(debug) << "Running DDS controller: static";
            staticMode = true;
        } else if (control == "dynamic" || control == "external" || control == "interactive") {
            LOG(debug) << "Running DDS controller: external";
        } else {
            LOG(error) << "Unrecognized control mode '" << control << "' requested. " << "Ignoring and falling back to static control mode.";
            staticMode = true;
        }

        SubscribeForCustomCommands();
        SubscribeForConnectingChannels();

        // subscribe to device state changes, pushing new state changes into the event queue
        SubscribeToDeviceStateChange([&](DeviceState newState) {
            fStateQueue.Push(newState);
            switch (newState) {
                case DeviceState::Bound:
                    // Receive addresses of connecting channels from DDS
                    // and propagate addresses of bound channels to DDS.
                    FillChannelContainers();

                    // publish bound addresses via DDS at keys corresponding to the channel
                    // prefixes, e.g. 'data' in data[i]
                    PublishBoundChannels();
                    break;
                case DeviceState::ResettingDevice: {
                    {
                        std::lock_guard<std::mutex> lk(fUpdateMutex);
                        fUpdatesAllowed = false;
                    }

                    EmptyChannelContainers();
                    break;
                }
                case DeviceState::Exiting:
                    fWorkGuard.reset();
                    fDeviceTerminationRequested = true;
                    UnsubscribeFromDeviceStateChange();
                    ReleaseDeviceControl();
                    break;
                default:
                    break;
            }

            lock_guard<mutex> lock{fStateChangeSubscriberMutex};
            string id = GetProperty<string>("id");
            fLastState = fCurrentState;
            fCurrentState = newState;
            for (auto subscriberId : fStateChangeSubscribers) {
                LOG(debug) << "Publishing state-change: " << fLastState << "->" << newState << " to " << subscriberId;
                fDDS.Send("state-change: " + id + "," + ToString(dds::env_prop<dds::task_id>()) + "," + ToStr(fLastState) + "->" + ToStr(newState), to_string(subscriberId));
            }
        });

        if (staticMode) {
            fControllerThread = thread(&DDS::StaticControl, this);
        } else {
            StartWorkerThread();
        }

        fDDS.Start();
    } catch (PluginServices::DeviceControlError& e) {
        LOG(debug) << e.what();
    } catch (exception& e) {
        LOG(error) << "Error in plugin initialization: " << e.what();
    }
}

void DDS::EmptyChannelContainers()
{
    fBindingChans.clear();
    fConnectingChans.clear();
}

auto DDS::StartWorkerThread() -> void
{
    fWorkerThread = thread([this]() {
        fWorkerQueue.run();
    });
}

auto DDS::WaitForExitingAck() -> void
{
    unique_lock<mutex> lock(fStateChangeSubscriberMutex);
    fExitingAcked.wait_for(
        lock,
        chrono::milliseconds(GetProperty<unsigned int>("wait-for-exiting-ack-timeout")),
        [this]() { return fExitingAckedByLastExternalController; });
}

auto DDS::StaticControl() -> void
{
    try {
        TransitionDeviceStateTo(DeviceState::Running);

        // wait until stop signal
        unique_lock<mutex> lock(fStopMutex);
        while (!fDeviceTerminationRequested) {
            fStopCondition.wait_for(lock, chrono::seconds(1));
        }
        LOG(debug) << "Stopping DDS plugin static controller";
    } catch (DeviceErrorState&) {
        ReleaseDeviceControl();
    } catch (exception& e) {
        ReleaseDeviceControl();
        LOG(error) << "Error: " << e.what() << endl;
        return;
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
            string chanName = vi.substr(0, pos);

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
        {
            std::lock_guard<std::mutex> lk(fUpdateMutex);
            fUpdatesAllowed = true;
        }
        fUpdateCondition.notify_one();
    } catch (const exception& e) {
        LOG(error) << "Error filling channel containers: " << e.what();
    }
}

auto DDS::SubscribeForConnectingChannels() -> void
{
    LOG(debug) << "Subscribing for DDS properties.";

    fDDS.SubscribeKeyValue([&] (const string& propertyId, const string& value, uint64_t senderTaskID) {
        LOG(debug) << "Received update for " << propertyId << ": value=" << value << ", senderTaskID=" << senderTaskID;

        boost::asio::post(fWorkerQueue, [=]() {
            try {
                {
                    std::unique_lock<std::mutex> lk(fUpdateMutex);
                    fUpdateCondition.wait(lk, [&]{ return fUpdatesAllowed; });
                }
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
                        fConnectingChans.at(propertyId).fDDSValues.insert({senderTaskID, connectionStrings.at(it2->second).c_str()});
                    } else {
                        LOG(error) << "multiple bound channels received, but no task index specified, only assigning the first";
                        fConnectingChans.at(propertyId).fDDSValues.insert({senderTaskID, connectionStrings.at(0).c_str()});
                    }
                } else { // only one bound channel received
                    fConnectingChans.at(propertyId).fDDSValues.insert({senderTaskID, val.c_str()});
                }

                // update channels and remove them from unfinished container
                for (auto mi = fConnectingChans.begin(); mi != fConnectingChans.end(); /* no increment */) {
                    if (mi->second.fSubChannelAddresses.size() == mi->second.fDDSValues.size()) {
                        // when multiple subChannels are used, their order on every device should be the same, irregardless of arrival order from DDS.
                        sort(mi->second.fSubChannelAddresses.begin(), mi->second.fSubChannelAddresses.end());
                        auto it3 = mi->second.fDDSValues.begin();
                        for (unsigned int i = 0; i < mi->second.fSubChannelAddresses.size(); ++i) {
                            SetProperty<string>(string{"chans." + mi->first + "." + to_string(i) + ".address"}, it3->second);
                            ++it3;
                        }
                        fConnectingChans.erase(mi++);
                    } else {
                        ++mi;
                    }
                }
            } catch (const exception& e) {
                LOG(error) << "Error on handling DDS property update for " << propertyId << ": value=" << value << ", senderTaskID=" << senderTaskID << ": " << e.what();
            }
        });
    });
}

auto DDS::PublishBoundChannels() -> void
{
    for (const auto& chan : fBindingChans) {
        string joined = boost::algorithm::join(chan.second, ",");
        LOG(debug) << "Publishing " << chan.first << " bound addresses (" << chan.second.size() << ") to DDS under '" << chan.first << "' property name.";
        fDDS.PutValue(chan.first, joined);
    }
}

auto DDS::HeartbeatSender() -> void
{
    string id = GetProperty<string>("id");

    while (!fDeviceTerminationRequested) {
        {
            lock_guard<mutex> lock{fHeartbeatSubscriberMutex};

            for (const auto subscriberId : fHeartbeatSubscribers) {
                fDDS.Send("heartbeat: " + id , to_string(subscriberId));
            }
        }

        this_thread::sleep_for(chrono::milliseconds(fHeartbeatInterval));
    }
}

auto DDS::SubscribeForCustomCommands() -> void
{
    LOG(debug) << "Subscribing for DDS custom commands.";

    string id = GetProperty<string>("id");

    fDDS.SubscribeCustomCmd([id, this](const string& cmd, const string& cond, uint64_t senderId) {
        LOG(info) << "Received command: '" << cmd << "' from " << senderId;

        if (cmd == "check-state") {
            fDDS.Send(id + ": " + ToStr(GetCurrentDeviceState()), to_string(senderId));
        } else if (fTransitions.find(cmd) != fTransitions.end()) {
            if (ChangeDeviceState(ToDeviceStateTransition(cmd))) {
                fDDS.Send(id + ": queued, " + cmd, to_string(senderId));
            } else {
                fDDS.Send(id + ": could not queue, " + cmd, to_string(senderId));
            }
            if (cmd == "END" && ToStr(GetCurrentDeviceState()) == "EXITING") {
                unique_lock<mutex> lock(fStopMutex);
                fStopCondition.notify_one();
            }
            {
                lock_guard<mutex> lock{fStateChangeSubscriberMutex};
                fLastExternalController = senderId;
            }
        } else if (cmd == "dump-config") {
            stringstream ss;
            for (const auto pKey: GetPropertyKeys()) {
                ss << id << ": " << pKey << " -> " << GetPropertyAsString(pKey) << endl;
            }
            fDDS.Send(ss.str(), to_string(senderId));
        } else if (cmd == "subscribe-to-heartbeats") {
            {
                // auto size = fHeartbeatSubscribers.size();
                lock_guard<mutex> lock{fHeartbeatSubscriberMutex};
                fHeartbeatSubscribers.insert(senderId);
            }
            fDDS.Send("heartbeat-subscription: " + id + ",OK", to_string(senderId));
        } else if (cmd == "unsubscribe-from-heartbeats") {
            {
                lock_guard<mutex> lock{fHeartbeatSubscriberMutex};
                fHeartbeatSubscribers.erase(senderId);
            }
            fDDS.Send("heartbeat-unsubscription: " + id + ",OK", to_string(senderId));
        } else if (cmd == "state-change-exiting-received") {
            {
                lock_guard<mutex> lock{fStateChangeSubscriberMutex};
                if (fLastExternalController == senderId) {
                    fExitingAckedByLastExternalController = true;
                }
            }
            fExitingAcked.notify_one();
        } else if (cmd == "subscribe-to-state-changes") {
            {
                // auto size = fStateChangeSubscribers.size();
                lock_guard<mutex> lock{fStateChangeSubscriberMutex};
                fStateChangeSubscribers.insert(senderId);
                if (!fControllerThread.joinable()) {
                    fControllerThread = thread(&DDS::WaitForExitingAck, this);
                }
            }
            fDDS.Send("state-changes-subscription: " + id + ",OK", to_string(senderId));
            {
                lock_guard<mutex> lock{fStateChangeSubscriberMutex};
                LOG(debug) << "Publishing state-change: " << fLastState << "->" << fCurrentState << " to " << senderId;
                // fDDSCustomCmd.send("state-change: " + id + "," + ToStr(fLastState) + "->" + ToStr(fCurrentState), to_string(senderId));
                fDDS.Send("state-change: " + id + "," + ToString(dds::env_prop<dds::task_id>()) + "," + ToStr(fLastState) + "->" + ToStr(fCurrentState), to_string(senderId));
            }
        } else if (cmd == "unsubscribe-from-state-changes") {
            {
                lock_guard<mutex> lock{fStateChangeSubscriberMutex};
                fStateChangeSubscribers.erase(senderId);
            }
            fDDS.Send("state-changes-unsubscription: " + id + ",OK", to_string(senderId));
        } else if (cmd == "SHUTDOWN") {
            TransitionDeviceStateTo(DeviceState::Exiting);
        } else if (cmd == "STARTUP") {
            TransitionDeviceStateTo(DeviceState::Running);
        } else {
            LOG(warn) << "Unknown command: " << cmd;
            LOG(warn) << "Origin: " << senderId;
            LOG(warn) << "Destination: " << cond;
        }
    });
}

DDS::~DDS()
{
    UnsubscribeFromDeviceStateChange();
    ReleaseDeviceControl();

    if (fControllerThread.joinable()) {
        fControllerThread.join();
    }

    if (fHeartbeatThread.joinable()) {
        fHeartbeatThread.join();
    }

    fWorkGuard.reset();
    if (fWorkerThread.joinable()) {
        fWorkerThread.join();
    }
}

} /* namespace plugins */
} /* namespace mq */
} /* namespace fair */
