/********************************************************************************
 *    Copyright (C) 2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "DDS.h"

#include <fairmq/sdk/commands/Commands.h>

#include <fairmq/tools/Strings.h>

#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/asio/post.hpp>

#include <cstdlib>
#include <stdexcept>
#include <sstream>

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

        string deviceId(GetProperty<string>("id"));
        if (deviceId.empty()) {
            SetProperty<string>("id", dds::env_prop<dds::task_path>());
        }
        string sessionId(GetProperty<string>("session"));
        if (sessionId == "default") {
            SetProperty<string>("session", dds::env_prop<dds::dds_session_id>());
        }

        auto control = GetProperty<string>("control");
        if (control == "static") {
            LOG(error) << "DDS Plugin: static mode is not supported";
            throw invalid_argument("DDS Plugin: static mode is not supported");
        } else if (control == "dynamic" || control == "external" || control == "interactive") {
            LOG(debug) << "Running DDS controller: external";
        } else {
            LOG(error) << "Unrecognized control mode '" << control << "' requested. " << "Ignoring and starting in external control mode.";
        }

        SubscribeForCustomCommands();
        SubscribeForConnectingChannels();

        // subscribe to device state changes, pushing new state changes into the event queue
        SubscribeToDeviceStateChange([&](DeviceState newState) {
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
                        lock_guard<mutex> lk(fUpdateMutex);
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
            using namespace sdk::cmd;
            for (auto subscriberId : fStateChangeSubscribers) {
                LOG(debug) << "Publishing state-change: " << fLastState << "->" << newState << " to " << subscriberId;

                Cmds cmds(make<StateChange>(id, dds::env_prop<dds::task_id>(), fLastState, fCurrentState));
                fDDS.Send(cmds.Serialize(), to_string(subscriberId));
            }
        });

        StartWorkerThread();

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
            lock_guard<mutex> lk(fUpdateMutex);
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

    fDDS.SubscribeKeyValue([&] (const string& key, const string& value, uint64_t senderTaskID) {
        LOG(debug) << "Received property: key=" << key << ", value=" << value << ", senderTaskID=" << senderTaskID;

        if (key.compare(0, 8, "fmqchan_") != 0) {
            LOG(debug) << "property update is not a channel info update: " << key;
            return;
        }
        string channelName = key.substr(8);
        LOG(info) << "Update for channel name: " << channelName;

        boost::asio::post(fWorkerQueue, [=]() {
            try {
                {
                    unique_lock<mutex> lk(fUpdateMutex);
                    fUpdateCondition.wait(lk, [&]{ return fUpdatesAllowed; });
                }
                string val = value;
                // check if it is to handle as one out of multiple values
                auto it = fIofN.find(channelName);
                if (it != fIofN.end()) {
                    it->second.fEntries.push_back(value);
                    if (it->second.fEntries.size() == it->second.fN) {
                        sort(it->second.fEntries.begin(), it->second.fEntries.end());
                        val = it->second.fEntries.at(it->second.fI);
                    } else {
                        LOG(debug) << "received " << it->second.fEntries.size() << " values for " << channelName << ", expecting total of " << it->second.fN;
                        return;
                    }
                }

                vector<string> connectionStrings;
                boost::algorithm::split(connectionStrings, val, boost::algorithm::is_any_of(","));
                if (connectionStrings.size() > 1) { // multiple bound channels received
                    auto it2 = fI.find(channelName);
                    if (it2 != fI.end()) {
                        LOG(debug) << "adding connecting channel " << channelName << " : " << connectionStrings.at(it2->second);
                        fConnectingChans.at(channelName).fDDSValues.insert({senderTaskID, connectionStrings.at(it2->second).c_str()});
                    } else {
                        LOG(error) << "multiple bound channels received, but no task index specified, only assigning the first";
                        fConnectingChans.at(channelName).fDDSValues.insert({senderTaskID, connectionStrings.at(0).c_str()});
                    }
                } else { // only one bound channel received
                    fConnectingChans.at(channelName).fDDSValues.insert({senderTaskID, val.c_str()});
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
                LOG(error) << "Error handling DDS property: key=" << key << ", value=" << value << ", senderTaskID=" << senderTaskID << ": " << e.what();
            }
        });
    });
}

auto DDS::PublishBoundChannels() -> void
{
    for (const auto& chan : fBindingChans) {
        string joined = boost::algorithm::join(chan.second, ",");
        LOG(debug) << "Publishing bound addresses (" << chan.second.size() << ") of channel '" << chan.first << "' to DDS under '" << "fmqchan_" + chan.first << "' property name.";
        fDDS.PutValue("fmqchan_" + chan.first, joined);
    }
}

auto DDS::HeartbeatSender() -> void
{
    using namespace sdk::cmd;
    string id = GetProperty<string>("id");

    while (!fDeviceTerminationRequested) {
        {
            lock_guard<mutex> lock{fHeartbeatSubscriberMutex};

            for (const auto subscriberId : fHeartbeatSubscribers) {
                fDDS.Send(Cmds(make<Heartbeat>(id)).Serialize(), to_string(subscriberId));
            }
        }

        this_thread::sleep_for(chrono::milliseconds(fHeartbeatInterval));
    }
}

auto DDS::SubscribeForCustomCommands() -> void
{
    LOG(debug) << "Subscribing for DDS custom commands.";

    string id = GetProperty<string>("id");

    fDDS.SubscribeCustomCmd([id, this](const string& cmdStr, const string& cond, uint64_t senderId) {
        // LOG(info) << "Received command: '" << cmdStr << "' from " << senderId;

        using namespace fair::mq::sdk;
        cmd::Cmds inCmds;
        inCmds.Deserialize(cmdStr);

        for (const auto& cmd : inCmds) {
            // LOG(info) << "Received command type: '" << cmd->GetType() << "' from " << senderId;
            switch (cmd->GetType()) {
                case cmd::Type::check_state: {
                    fDDS.Send(cmd::Cmds(cmd::make<cmd::CurrentState>(id, GetCurrentDeviceState()))
                                  .Serialize(),
                              to_string(senderId));
                } break;
                case cmd::Type::change_state: {
                    Transition transition = static_cast<cmd::ChangeState&>(*cmd).GetTransition();
                    if (ChangeDeviceState(transition)) {
                        cmd::Cmds outCmds(
                            cmd::make<cmd::TransitionStatus>(id, dds::env_prop<dds::task_id>(), cmd::Result::Ok, transition));
                        fDDS.Send(outCmds.Serialize(), to_string(senderId));
                    } else {
                        sdk::cmd::Cmds outCmds(
                            cmd::make<cmd::TransitionStatus>(id, dds::env_prop<dds::task_id>(), cmd::Result::Failure, transition));
                        fDDS.Send(outCmds.Serialize(), to_string(senderId));
                    }
                    {
                        lock_guard<mutex> lock{fStateChangeSubscriberMutex};
                        fLastExternalController = senderId;
                    }
                } break;
                case cmd::Type::dump_config: {
                    stringstream ss;
                    for (const auto pKey : GetPropertyKeys()) {
                        ss << id << ": " << pKey << " -> " << GetPropertyAsString(pKey) << "\n";
                    }
                    cmd::Cmds outCmds(cmd::make<cmd::Config>(id, ss.str()));
                    fDDS.Send(outCmds.Serialize(), to_string(senderId));
                } break;
                case cmd::Type::subscribe_to_heartbeats: {
                    {
                        lock_guard<mutex> lock{fHeartbeatSubscriberMutex};
                        fHeartbeatSubscribers.insert(senderId);
                    }
                    cmd::Cmds outCmds(cmd::make<cmd::HeartbeatSubscription>(id, cmd::Result::Ok));
                    fDDS.Send(outCmds.Serialize(), to_string(senderId));
                } break;
                case cmd::Type::unsubscribe_from_heartbeats: {
                    {
                        lock_guard<mutex> lock{fHeartbeatSubscriberMutex};
                        fHeartbeatSubscribers.erase(senderId);
                    }
                    cmd::Cmds outCmds(cmd::make<cmd::HeartbeatUnsubscription>(id, cmd::Result::Ok));
                    fDDS.Send(outCmds.Serialize(), to_string(senderId));
                } break;
                case cmd::Type::state_change_exiting_received: {
                    {
                        lock_guard<mutex> lock{fStateChangeSubscriberMutex};
                        if (fLastExternalController == senderId) {
                            fExitingAckedByLastExternalController = true;
                        }
                    }
                    fExitingAcked.notify_one();
                } break;
                case cmd::Type::subscribe_to_state_change: {
                    lock_guard<mutex> lock{fStateChangeSubscriberMutex};
                    fStateChangeSubscribers.insert(senderId);
                    if (!fControllerThread.joinable()) {
                        fControllerThread = thread(&DDS::WaitForExitingAck, this);
                    }

                    LOG(debug) << "Publishing state-change: " << fLastState << "->" << fCurrentState
                               << " to " << senderId;

                    cmd::Cmds outCmds(
                        cmd::make<cmd::StateChangeSubscription>(id, cmd::Result::Ok),
                        cmd::make<cmd::StateChange>(
                            id, dds::env_prop<dds::task_id>(), fLastState, fCurrentState));

                    fDDS.Send(outCmds.Serialize(), to_string(senderId));
                } break;
                case cmd::Type::unsubscribe_from_state_change: {
                    {
                        lock_guard<mutex> lock{fStateChangeSubscriberMutex};
                        fStateChangeSubscribers.erase(senderId);
                    }
                    cmd::Cmds outCmds(
                        cmd::make<cmd::StateChangeUnsubscription>(id, cmd::Result::Ok));
                    fDDS.Send(outCmds.Serialize(), to_string(senderId));
                } break;
                case cmd::Type::get_properties: {
                    auto _cmd = static_cast<cmd::GetProperties&>(*cmd);
                    auto const request_id(_cmd.GetRequestId());
                    auto result(cmd::Result::Ok);
                    std::vector<std::pair<std::string, std::string>> props;
                    try {
                        for (auto const& prop : GetPropertiesAsString(_cmd.GetQuery())) {
                            props.push_back({prop.first, prop.second});
                        }
                    } catch (std::exception const& e) {
                        LOG(warn) << "Getting properties (request id: " << request_id << ") failed: " << e.what();
                        result = cmd::Result::Failure;
                    }
                    cmd::Cmds const outCmds(cmd::make<cmd::Properties>(id, request_id, result, props));
                    fDDS.Send(outCmds.Serialize(), to_string(senderId));
                } break;
                case cmd::Type::set_properties: {
                    auto _cmd(static_cast<cmd::SetProperties&>(*cmd));
                    auto const request_id(_cmd.GetRequestId());
                    auto result(cmd::Result::Ok);
                    try {
                        fair::mq::Properties props;
                        for (auto const& prop : _cmd.GetProps()) {
                            props.insert({prop.first, fair::mq::Property(prop.second)});
                        }
                        // TODO Handle builtin keys with different value type than string
                        SetProperties(props);
                    } catch (std::exception const& e) {
                        LOG(warn) << "Setting properties (request id: " << request_id << ") failed: " << e.what();
                        result = cmd::Result::Failure;
                    }
                    cmd::Cmds const outCmds(cmd::make<cmd::PropertiesSet>(id, request_id, result));
                    fDDS.Send(outCmds.Serialize(), to_string(senderId));
                } break;
                default:
                    LOG(warn) << "Unexpected/unknown command received: " << cmdStr;
                    LOG(warn) << "Origin: " << senderId;
                    LOG(warn) << "Destination: " << cond;
                break;
            }
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
