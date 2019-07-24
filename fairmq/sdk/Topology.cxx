/********************************************************************************
 *    Copyright (C) 2019 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "Topology.h"

#include <fairlogger/Logger.h>

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>

#include <utility>
#include <thread>
#include <future>
#include <mutex>
#include <condition_variable>

namespace fair {
namespace mq {

auto operator<<(std::ostream& os, AsyncOpResultCode v) -> std::ostream&
{
    switch (v) {
        case AsyncOpResultCode::Aborted:
            return os << "Aborted";
        case AsyncOpResultCode::Timeout:
            return os << "Timeout";
        case AsyncOpResultCode::Error:
            return os << "Error";
        case AsyncOpResultCode::Ok:
        default:
            return os << "Ok";
    }
}

auto operator<<(std::ostream& os, AsyncOpResult v) -> std::ostream&
{
    (void)(os << "[" << v.code << "]");
    if (v.msg.empty()) {
        (void)(os << " " << v.msg);
    }
    return os;
}

namespace sdk {

const std::unordered_map<DeviceTransition, DeviceState, tools::HashEnum<DeviceTransition>>
    expectedState = {{Transition::InitDevice,   DeviceState::InitializingDevice},
                     {Transition::CompleteInit, DeviceState::Initialized},
                     {Transition::Bind,         DeviceState::Bound},
                     {Transition::Connect,      DeviceState::DeviceReady},
                     {Transition::InitTask,     DeviceState::InitializingTask},
                     {Transition::Run,          DeviceState::Running},
                     {Transition::Stop,         DeviceState::Ready},
                     {Transition::ResetTask,    DeviceState::DeviceReady},
                     {Transition::ResetDevice,  DeviceState::Idle},
                     {Transition::End,          DeviceState::Exiting}};

Topology::Topology(DDSTopology topo, DDSSession session)
    : fDDSSession(std::move(session))
    , fDDSTopo(std::move(topo))
    , fStateChangeOngoing(false)
    , fTargetState(DeviceState::Idle)
    , fStateChangeTimeout(0)
    , fShutdown(false)
{
    std::vector<uint64_t> deviceList = fDDSTopo.GetDeviceList();
    for (const auto& d : deviceList) {
        LOG(info) << "fair::mq::Topology Adding device " << d;
        fTopologyState.emplace(d, DeviceStatus{ false, DeviceState::Ok });
    }
    fDDSSession.SubscribeToCommands([this](const std::string& msg, const std::string& /* condition */, uint64_t senderId) {
        LOG(debug) << "Received from " << senderId << ": " << msg;
        std::vector<std::string> parts;
        boost::algorithm::split(parts, msg, boost::algorithm::is_any_of(":,"));

        for (unsigned int i = 0; i < parts.size(); ++i) {
            boost::trim(parts.at(i));
        }

        if (parts[0] == "state-change") {
            boost::trim(parts[3]);
            AddNewStateEntry(std::stoull(parts[2]), parts[3]);
        } else if (parts[0] == "state-changes-subscription") {
            if (parts[2] != "OK") {
                LOG(error) << "state-changes-subscription failed with return code: " << parts[2];
            }
        } else if (parts[0] == "state-changes-unsubscription") {
            if (parts[2] != "OK") {
                LOG(error) << "state-changes-unsubscription failed with return code: " << parts[2];
            }
        }
    });
    fDDSSession.StartDDSService();
    fDDSSession.SendCommand("subscribe-to-state-changes");

    fExecutionThread = std::thread(&Topology::WaitForState, this);
}

auto Topology::ChangeState(TopologyTransition transition, ChangeStateCallback cb, Duration timeout) -> void
{
    {
        std::lock_guard<std::mutex> guard(fMtx);
        if (fStateChangeOngoing) {
            LOG(error) << "State change already in progress, concurrent requested not yet supported";
            return; // TODO call the callback with error msg
        }
        LOG(info) << "Initiating ChangeState with " << transition << " to " << expectedState.at(transition);
        fStateChangeOngoing = true;
        fChangeStateCallback = cb;
        fStateChangeTimeout = timeout;
        fTargetState = expectedState.at(transition);

        fDDSSession.SendCommand(GetTransitionName(transition));
    }
    fExecutionCV.notify_one();
}

auto Topology::ChangeState(TopologyTransition t, Duration timeout) -> ChangeStateResult
{
    fair::mq::tools::Semaphore blocker;
    ChangeStateResult res;
    ChangeState(
        t,
        [&blocker, &res](Topology::ChangeStateResult _res) {
            res = _res;
            blocker.Signal();
        },
        timeout);
    blocker.Wait();
    return res;
}

void Topology::WaitForState()
{
    while (!fShutdown) {
        if (fStateChangeOngoing) {
            try {
                auto condition = [&] {
                    // LOG(info) << "checking condition";
                    // LOG(info) << "fShutdown: " << fShutdown;
                    // LOG(info) << "condition: " << std::all_of(fTopologyState.cbegin(), fTopologyState.cend(), [&](TopologyState::value_type i) { return i.second.state == fTargetState; });
                    return fShutdown || std::all_of(fTopologyState.cbegin(), fTopologyState.cend(), [&](TopologyState::value_type i) {
                        return i.second.state == fTargetState;
                    });
                };

                std::unique_lock<std::mutex> lock(fMtx);

                // TODO Fix the timeout version
                if (fStateChangeTimeout > std::chrono::milliseconds(0)) {
                    LOG(debug) << "initiating wait with timeout";
                    if (!fCV.wait_for(lock, fStateChangeTimeout, condition)) {
                        LOG(debug) << "timeout";
                        fStateChangeOngoing = false;
                        break;
                    }
                } else {
                    LOG(debug) << "initiating wait without timeout";
                    fCV.wait(lock, condition);
                }

                fStateChangeOngoing = false;
                if (fShutdown) {
                    // TODO call the callback here with Aborted result
                    break;
                }
            } catch(std::exception& e) {
                LOG(error) << "Error while processing state request: " << e.what();
                fChangeStateCallback({{AsyncOpResultCode::Error, ""}, fTopologyState});
            }

            fChangeStateCallback({{AsyncOpResultCode::Ok, ""}, fTopologyState});
        } else {
            std::unique_lock<std::mutex> lock(fExecutionMtx);
            fExecutionCV.wait(lock);
        }
    }
    LOG(debug) << "WaitForState shutting down";
};

void Topology::AddNewStateEntry(uint64_t senderId, const std::string& state)
{
    std::size_t pos = state.find("->");
    std::string endState = state.substr(pos + 2);
    LOG(info) << "Adding new state entry: " << senderId << ", " << state << ", end state: " << endState;
    {
        try {
            std::unique_lock<std::mutex> lock(fMtx);
            fTopologyState[senderId] = DeviceStatus{ true, fair::mq::GetState(endState) };
        } catch(const std::exception& e) {
            LOG(error) << "Exception in AddNewStateEntry: " << e.what();
        }

        LOG(info) << "fTopologyState after update: ";
        for (auto& e : fTopologyState) {
            LOG(info) << e.first << ": " << e.second.state;
        }
    }
    fCV.notify_one();
}

Topology::~Topology()
{
    fDDSSession.UnsubscribeFromCommands();
    {
        std::lock_guard<std::mutex> guard(fMtx);
        fShutdown = true;
    }
    fExecutionCV.notify_one();
    fExecutionThread.join();
}

auto operator<<(std::ostream& os, Topology::ChangeStateResult v) -> std::ostream&
{
    return os << v.rc;
}

}   // namespace sdk
}   // namespace mq
}   // namespace fair
