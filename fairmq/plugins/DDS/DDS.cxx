/********************************************************************************
 *    Copyright (C) 2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "DDS.h"

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
    , fDDSTaskId(dds::env_prop<dds::task_id>())
    , fCurrentState(DeviceState::Idle)
    , fLastState(DeviceState::Idle)
    , fDeviceTerminationRequested(false)
    , fLastExternalController(0)
    , fExitingAckedByLastExternalController(false)
    , fUpdatesAllowed(false)
    , fWorkGuard(fWorkerQueue.get_executor())
{
    try {
        TakeDeviceControl();

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
                case DeviceState::Bound: {
                    // Receive addresses of connecting channels from DDS
                    // and propagate addresses of bound channels to DDS.
                    FillChannelContainers();

                    // allow updates from key value after channel containers are filled
                    {
                        lock_guard<mutex> lk(fUpdateMutex);
                        fUpdatesAllowed = true;
                    }
                    fUpdateCondition.notify_one();

                    // publish bound addresses via DDS at keys corresponding to the channel
                    // prefixes, e.g. 'data' in data[i]
                    PublishBoundChannels();
                } break;
                case DeviceState::ResettingDevice: {
                    {
                        lock_guard<mutex> lk(fUpdateMutex);
                        fUpdatesAllowed = false;
                    }

                    EmptyChannelContainers();
                } break;
                case DeviceState::Exiting: {
                    if (!fControllerThread.joinable()) {
                        fControllerThread = thread(&DDS::WaitForExitingAck, this);
                    }
                    fWorkGuard.reset();
                    fDeviceTerminationRequested = true;
                    UnsubscribeFromDeviceStateChange();
                    ReleaseDeviceControl();
                } break;
                default:
                    break;
            }

            using namespace sdk::cmd;
            auto now = chrono::steady_clock::now();
            string id = GetProperty<string>("id");
            fLastState = fCurrentState;
            fCurrentState = newState;

            lock_guard<mutex> lock{fStateChangeSubscriberMutex};
            for (auto it = fStateChangeSubscribers.cbegin(); it != fStateChangeSubscribers.end();) {
                // if a subscriber did not send a heartbeat in more than 3 times the promised interval,
                // remove it from the subscriber list
                if (chrono::duration<double>(now - it->second.first).count() > 3 * it->second.second) {
                    LOG(warn) << "Controller '" << it->first
                              << "' did not send heartbeats since over 3 intervals ("
                              << 3 * it->second.second << " ms), removing it.";
                    fStateChangeSubscribers.erase(it++);
                } else {
                    LOG(debug) << "Publishing state-change: " << fLastState << "->" << fCurrentState << " to " << it->first;
                    Cmds cmds(make<StateChange>(id, fDDSTaskId, fLastState, fCurrentState));
                    fDDS.Send(cmds.Serialize(), to_string(it->first));
                    ++it;
                }
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
    auto timeout = GetProperty<unsigned int>("wait-for-exiting-ack-timeout");
    fExitingAcked.wait_for(lock, chrono::milliseconds(timeout), [this]() {
        return fExitingAckedByLastExternalController || fStateChangeSubscribers.empty();
    });
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
                fConnectingChans.at(c.first).fNumSubChannels = c.second;
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

                if (fConnectingChans.find(channelName) == fConnectingChans.end()) {
                    LOG(error) << "Received an update for a connecting channel, but either no channel with given channel name exists or it has already been configured: '" << channelName << "', ignoring...";
                    return;
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

                for (const auto& mi : fConnectingChans) {
                    if (mi.second.fNumSubChannels == mi.second.fDDSValues.size()) {
                        int i = 0;
                        for (const auto& e : mi.second.fDDSValues) {
                            auto result = UpdateProperty<string>(string{"chans." + mi.first + "." + to_string(i) + ".address"}, e.second);
                            if (!result) {
                                LOG(error) << "UpdateProperty failed for: " << "chans." << mi.first << "." << to_string(i) << ".address" << " - property does not exist";
                            }
                            ++i;
                        }
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

auto DDS::SubscribeForCustomCommands() -> void
{
    LOG(debug) << "Subscribing for DDS custom commands.";

    string id = GetProperty<string>("id");

    fDDS.SubscribeCustomCmd([id, this](const string& cmdStr, const string& cond, uint64_t senderId) {
        // LOG(info) << "Received command: '" << cmdStr << "' from " << senderId;
        sdk::cmd::Cmds inCmds;
        inCmds.Deserialize(cmdStr);
        for (const auto& cmd : inCmds) {
            HandleCmd(id, *cmd, cond, senderId);
        }
    });
}

auto DDS::HandleCmd(const string& id, sdk::cmd::Cmd& cmd, const string& cond, uint64_t senderId) -> void
{
    using namespace fair::mq::sdk;
    using namespace fair::mq::sdk::cmd;
    // LOG(info) << "Received command type: '" << cmd.GetType() << "' from " << senderId;
    switch (cmd.GetType()) {
        case Type::check_state: {
            fDDS.Send(Cmds(make<CurrentState>(id, GetCurrentDeviceState())).Serialize(), to_string(senderId));
        } break;
        case Type::change_state: {
            Transition transition = static_cast<ChangeState&>(cmd).GetTransition();
            if (ChangeDeviceState(transition)) {
                Cmds outCmds(make<TransitionStatus>(id, fDDSTaskId, Result::Ok, transition, GetCurrentDeviceState()));
                fDDS.Send(outCmds.Serialize(), to_string(senderId));
            } else {
                Cmds outCmds(make<TransitionStatus>(id, fDDSTaskId, Result::Failure, transition, GetCurrentDeviceState()));
                fDDS.Send(outCmds.Serialize(), to_string(senderId));
            }
            {
                lock_guard<mutex> lock{fStateChangeSubscriberMutex};
                fLastExternalController = senderId;
            }
        } break;
        case Type::dump_config: {
            stringstream ss;
            for (const auto pKey : GetPropertyKeys()) {
                ss << id << ": " << pKey << " -> " << GetPropertyAsString(pKey) << "\n";
            }
            Cmds outCmds(make<Config>(id, ss.str()));
            fDDS.Send(outCmds.Serialize(), to_string(senderId));
        } break;
        case Type::state_change_exiting_received: {
            {
                lock_guard<mutex> lock{fStateChangeSubscriberMutex};
                if (fLastExternalController == senderId) {
                    fExitingAckedByLastExternalController = true;
                }
            }
            fExitingAcked.notify_one();
        } break;
        case Type::subscribe_to_state_change: {
            auto _cmd = static_cast<cmd::SubscribeToStateChange&>(cmd);
            lock_guard<mutex> lock{fStateChangeSubscriberMutex};
            fStateChangeSubscribers.emplace(senderId, make_pair(chrono::steady_clock::now(), _cmd.GetInterval()));

            LOG(debug) << "Publishing state-change: " << fLastState << "->" << fCurrentState << " to " << senderId;

            Cmds outCmds(make<StateChangeSubscription>(id, fDDSTaskId, Result::Ok),
                         make<StateChange>(id, fDDSTaskId, fLastState, fCurrentState));

            fDDS.Send(outCmds.Serialize(), to_string(senderId));
        } break;
        case Type::subscription_heartbeat: {
            try {
                auto _cmd = static_cast<cmd::SubscriptionHeartbeat&>(cmd);
                lock_guard<mutex> lock{fStateChangeSubscriberMutex};
                fStateChangeSubscribers.at(senderId) = make_pair(chrono::steady_clock::now(), _cmd.GetInterval());
            } catch(out_of_range& oor) {
                LOG(warn) << "Received subscription heartbeat from an unknown controller with id '" << senderId << "'";
            }
        } break;
        case Type::unsubscribe_from_state_change: {
            {
                lock_guard<mutex> lock{fStateChangeSubscriberMutex};
                fStateChangeSubscribers.erase(senderId);
            }
            Cmds outCmds(make<StateChangeUnsubscription>(id, fDDSTaskId, Result::Ok));
            fDDS.Send(outCmds.Serialize(), to_string(senderId));
        } break;
        case Type::get_properties: {
            auto _cmd = static_cast<cmd::GetProperties&>(cmd);
            auto const request_id(_cmd.GetRequestId());
            auto result(Result::Ok);
            vector<pair<string, string>> props;
            try {
                for (auto const& prop : GetPropertiesAsString(_cmd.GetQuery())) {
                    props.push_back({prop.first, prop.second});
                }
            } catch (exception const& e) {
                LOG(warn) << "Getting properties (request id: " << request_id << ") failed: " << e.what();
                result = Result::Failure;
            }
            Cmds const outCmds(make<cmd::Properties>(id, request_id, result, props));
            fDDS.Send(outCmds.Serialize(), to_string(senderId));
        } break;
        case Type::set_properties: {
            auto _cmd(static_cast<cmd::SetProperties&>(cmd));
            auto const request_id(_cmd.GetRequestId());
            auto result(Result::Ok);
            try {
                fair::mq::Properties props;
                for (auto const& prop : _cmd.GetProps()) {
                    props.insert({prop.first, fair::mq::Property(prop.second)});
                }
                // TODO Handle builtin keys with different value type than string
                SetProperties(props);
            } catch (exception const& e) {
                LOG(warn) << "Setting properties (request id: " << request_id << ") failed: " << e.what();
                result = Result::Failure;
            }
            Cmds const outCmds(make<PropertiesSet>(id, request_id, result));
            fDDS.Send(outCmds.Serialize(), to_string(senderId));
        } break;
        default:
            LOG(warn) << "Unexpected/unknown command received: " << cmd.GetType();
            LOG(warn) << "Origin: " << senderId;
            LOG(warn) << "Destination: " << cond;
        break;
            }
}

DDS::~DDS()
{
    UnsubscribeFromDeviceStateChange();
    ReleaseDeviceControl();

    if (fControllerThread.joinable()) {
        fControllerThread.join();
    }

    fWorkGuard.reset();
    if (fWorkerThread.joinable()) {
        fWorkerThread.join();
    }
}

} /* namespace plugins */
} /* namespace mq */
} /* namespace fair */
