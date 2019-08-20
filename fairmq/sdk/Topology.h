/********************************************************************************
 *    Copyright (C) 2019 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_SDK_TOPOLOGY_H
#define FAIR_MQ_SDK_TOPOLOGY_H

#include <chrono>
#include <fairmq/States.h>
#include <fairmq/Tools.h>
#include <fairmq/sdk/DDSInfo.h>
#include <fairmq/sdk/DDSSession.h>
#include <fairmq/sdk/DDSTopology.h>
#include <fairmq/sdk/Error.h>
#include <functional>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace fair {
namespace mq {

enum class AsyncOpResultCode
{
    Ok,
    Timeout,
    Error,
    Aborted
};
auto operator<<(std::ostream& os, AsyncOpResultCode v) -> std::ostream&;

using AsyncOpResultMessage = std::string;

struct AsyncOpResult {
    AsyncOpResultCode code;
    AsyncOpResultMessage msg;
    operator AsyncOpResultCode() const { return code; }
};
auto operator<<(std::ostream& os, AsyncOpResult v) -> std::ostream&;

namespace sdk {

using DeviceState = fair::mq::State;
using DeviceTransition = fair::mq::Transition;

struct DeviceStatus
{
    bool initialized;
    DeviceState state;
};

using TopologyState = std::unordered_map<DDSTask::Id, DeviceStatus>;
using TopologyTransition = fair::mq::Transition;

struct MixedState : std::runtime_error { using std::runtime_error::runtime_error; };

inline DeviceState AggregateState(const TopologyState& topologyState)
{
    DeviceState first = topologyState.begin()->second.state;

    if (std::all_of(topologyState.cbegin(), topologyState.cend(), [&](TopologyState::value_type i) {
            return i.second.state == first;
        })) {
        return first;
    }

    throw MixedState("State is not uniform");

}

inline bool StateEqualsTo(const TopologyState& topologyState, DeviceState state)
{
    return AggregateState(topologyState) == state;
}

/**
 * @class Topology Topology.h <fairmq/sdk/Topology.h>
 * @brief Represents a FairMQ topology
 */
class Topology
{
  public:
    /// @brief (Re)Construct a FairMQ topology from an existing DDS topology
    /// @param topo DDSTopology
    /// @param session DDSSession
    explicit Topology(DDSTopology topo, DDSSession session = DDSSession());

    /// @brief (Re)Construct a FairMQ topology based on already existing native DDS API objects
    /// @param nativeSession Existing and initialized CSession (either via create() or attach())
    /// @param nativeTopo Existing CTopology that is activated on the given nativeSession
    /// @param env Optional DDSEnv (needed primarily for unit testing)
    explicit Topology(dds::topology_api::CTopology nativeTopo,
                      std::shared_ptr<dds::tools_api::CSession> nativeSession,
                      DDSEnv env = {});

    explicit Topology(const Topology&) = delete;
    Topology& operator=(const Topology&) = delete;
    explicit Topology(Topology&&) = delete;
    Topology& operator=(Topology&&) = delete;

    ~Topology();

    struct ChangeStateResult {
        AsyncOpResult rc;
        TopologyState state;
        friend auto operator<<(std::ostream& os, ChangeStateResult v) -> std::ostream&;
    };
    using ChangeStateCallback = std::function<void(ChangeStateResult)>;
    using Duration = std::chrono::milliseconds;

    /// @brief Initiate state transition on all FairMQ devices in this topology
    /// @param t FairMQ device state machine transition
    /// @param cb Completion callback
    /// @param timeout Timeout in milliseconds, 0 means no timeout
    auto ChangeState(TopologyTransition t, ChangeStateCallback cb, Duration timeout = std::chrono::milliseconds(0)) -> void;

    /// @brief Perform a state transition on all FairMQ devices in this topology
    /// @param t FairMQ device state machine transition
    /// @param timeout Timeout in milliseconds, 0 means no timeout
    /// @return The result of the state transition
    auto ChangeState(TopologyTransition t, Duration timeout = std::chrono::milliseconds(0)) -> ChangeStateResult;

    /// @brief Returns the current state of the topology
    /// @return map of id : DeviceStatus (initialized, state)
    TopologyState GetCurrentState() const { std::lock_guard<std::mutex> guard(fMtx); return fState; }

    DeviceState AggregateState() { return sdk::AggregateState(fState); }

    bool StateEqualsTo(DeviceState state) { return sdk::StateEqualsTo(fState, state); }

  private:
    DDSSession fDDSSession;
    DDSTopology fDDSTopo;
    TopologyState fState;
    bool fStateChangeOngoing;
    DeviceState fTargetState;
    mutable std::mutex fMtx;
    mutable std::mutex fExecutionMtx;
    std::condition_variable fCV;
    std::condition_variable fExecutionCV;
    std::thread fExecutionThread;
    ChangeStateCallback fChangeStateCallback;
    std::chrono::milliseconds fStateChangeTimeout;
    bool fShutdown;
    std::string fStateChangeError;

    void WaitForState();
    void AddNewStateEntry(DDSTask::Id taskId, const std::string& state);
};

using Topo = Topology;

}   // namespace sdk
}   // namespace mq
}   // namespace fair

#endif /* FAIR_MQ_SDK_TOPOLOGY_H */
