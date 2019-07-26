/********************************************************************************
 *    Copyright (C) 2019 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "Topology.h"

#include <DDS/Tools.h>
#include <DDS/Topology.h>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <condition_variable>
#include <fairlogger/Logger.h>
#include <future>
#include <mutex>
#include <thread>
#include <utility>

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
    os << "[" << v.code << "]";
    if (!v.msg.empty()) {
        os << " " << v.msg;
    }
    return os;
}

namespace sdk {

const std::unordered_map<DeviceTransition, DeviceState, tools::HashEnum<DeviceTransition>> expectedState =
{
    { Transition::InitDevice,   DeviceState::InitializingDevice },
    { Transition::CompleteInit, DeviceState::Initialized },
    { Transition::Bind,         DeviceState::Bound },
    { Transition::Connect,      DeviceState::DeviceReady },
    { Transition::InitTask,     DeviceState::InitializingTask },
    { Transition::Run,          DeviceState::Running },
    { Transition::Stop,         DeviceState::Ready },
    { Transition::ResetTask,    DeviceState::DeviceReady },
    { Transition::ResetDevice,  DeviceState::Idle },
    { Transition::End,          DeviceState::Exiting }
};

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
        // LOG(debug) << "Adding device " << d;
        fState.emplace(d, DeviceStatus{ false, DeviceState::Ok });
    }
    fDDSSession.SubscribeToCommands([this](const std::string& msg, const std::string& /* condition */, uint64_t senderId) {
        LOG(debug) << "Received from " << senderId << ": " << msg;
        std::vector<std::string> parts;
        boost::algorithm::split(parts, msg, boost::algorithm::is_any_of(":,"));

        for (unsigned int i = 0; i < parts.size(); ++i) {
            boost::trim(parts.at(i));
        }

        if (parts[0] == "state-change") {
            AddNewStateEntry(std::stoull(parts[2]), parts[3]);
        } else if (parts[0] == "state-changes-subscription") {
            if (parts[2] != "OK") {
                LOG(error) << "state-changes-subscription failed with return code: " << parts[2];
            }
        } else if (parts[0] == "state-changes-unsubscription") {
            if (parts[2] != "OK") {
                LOG(error) << "state-changes-unsubscription failed with return code: " << parts[2];
            }
        } else if (parts[1] == "could not queue") {
            LOG(warn) << "Could not queue " << parts[2] << " transition on " << senderId;
        }
    });
    fDDSSession.StartDDSService();
    fDDSSession.SendCommand("subscribe-to-state-changes");

    fExecutionThread = std::thread(&Topology::WaitForState, this);
}

Topology::Topology(dds::topology_api::CTopology nativeTopo,
                   std::shared_ptr<dds::tools_api::CSession> nativeSession,
                   DDSEnv env)
    : Topology(DDSTopo(std::move(nativeTopo), env), DDSSession(std::move(nativeSession), env))
{
    if (fDDSSession.RequestCommanderInfo().activeTopologyName != fDDSTopo.GetName()) {
        throw std::runtime_error("Given topology must be activated");
    }
}

auto Topology::ChangeState(TopologyTransition transition, ChangeStateCallback cb, Duration timeout) -> void
{
    {
        std::unique_lock<std::mutex> lock(fMtx);
        if (fStateChangeOngoing) {
            throw std::runtime_error("A state change request is already in progress, concurrent requests are currently not supported");
            lock.unlock();
        }
        LOG(debug) << "Initiating ChangeState with " << transition << " to " << expectedState.at(transition);
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
                    // LOG(info) << "condition: " << std::all_of(fState.cbegin(), fState.cend(), [&](TopologyState::value_type i) { return i.second.state == fTargetState; });
                    return fShutdown || std::all_of(fState.cbegin(), fState.cend(), [&](TopologyState::value_type i) {
                        return i.second.state == fTargetState;
                    });
                };

                std::unique_lock<std::mutex> lock(fMtx);

                if (fStateChangeTimeout > std::chrono::milliseconds(0)) {
                    if (!fCV.wait_for(lock, fStateChangeTimeout, condition)) {
                        // LOG(debug) << "timeout";
                        fStateChangeOngoing = false;
                        TopologyState state = fState;
                        lock.unlock();
                        fChangeStateCallback({{AsyncOpResultCode::Timeout, "timeout"}, std::move(state)});
                        break;
                    }
                } else {
                    fCV.wait(lock, condition);
                }

                fStateChangeOngoing = false;
                if (fShutdown) {
                    LOG(debug) << "Aborting because a shutdown was requested";
                    TopologyState state = fState;
                    lock.unlock();
                    fChangeStateCallback({{AsyncOpResultCode::Aborted, "Aborted because a shutdown was requested"}, std::move(state)});
                    break;
                }
            } catch (std::exception& e) {
                fStateChangeOngoing = false;
                LOG(error) << "Error while processing state request: " << e.what();
                fChangeStateCallback({{AsyncOpResultCode::Error, tools::ToString("Exception thrown: ", e.what())}, fState});
            }

            fChangeStateCallback({{AsyncOpResultCode::Ok, "success"}, fState});
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
    // LOG(debug) << "Adding new state entry: " << senderId << ", " << state << ", end state: " << endState;
    {
        try {
            std::unique_lock<std::mutex> lock(fMtx);
            fState[senderId] = DeviceStatus{ true, fair::mq::GetState(endState) };
        } catch (const std::exception& e) {
            LOG(error) << "Exception in AddNewStateEntry: " << e.what();
        }

        // LOG(info) << "fState after update: ";
        // for (auto& e : fState) {
        //     LOG(info) << e.first << ": " << e.second.state;
        // }
    }
    fCV.notify_one();
}

Topology::~Topology()
{
    fDDSSession.UnsubscribeFromCommands();
    {
        std::lock_guard<std::mutex> guard(fExecutionMtx);
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
