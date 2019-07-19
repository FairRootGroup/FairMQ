/********************************************************************************
 *    Copyright (C) 2019 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_SDK_TOPOLOGY_H
#define FAIR_MQ_SDK_TOPOLOGY_H

#include <fairmq/sdk/DDSInfo.h>
#include <fairmq/sdk/DDSSession.h>
#include <fairmq/sdk/DDSTopology.h>
#include <fairmq/States.h>
#include <fairmq/Tools.h>

#include <functional>
#include <unordered_map>
#include <memory>
#include <chrono>
#include <ostream>
#include <string>
#include <vector>

namespace fair {
namespace mq {

enum class AsyncOpResult {
    Ok,
    Timeout,
    Error,
    Aborted
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

using TopologyState = std::unordered_map<uint64_t, DeviceStatus>;
using TopologyTransition = fair::mq::Transition;

/**
 * @class Topology Topology.h <fairmq/sdk/Topology.h>
 * @brief Represents a FairMQ topology
 */
class Topology
{
  public:
    /// @brief (Re)Construct a FairMQ topology from an existing DDS topology
    /// @param topo Initialized DDS CTopology
    explicit Topology(DDSTopology topo, DDSSession session = DDSSession());
    ~Topology();

    struct ChangeStateResult {
        AsyncOpResult rc;
        TopologyState state;
        friend auto operator<<(std::ostream& os, ChangeStateResult v) -> std::ostream&;
    };
    using ChangeStateCallback = std::function<void(ChangeStateResult)>;

    /// @brief Initiate state transition on all FairMQ devices in this topology
    /// @param t FairMQ device state machine transition
    /// @param cb Completion callback
    auto ChangeState(TopologyTransition t, ChangeStateCallback cb, const std::chrono::milliseconds& timeout = std::chrono::milliseconds(0)) -> void;

    static const std::unordered_map<DeviceTransition, DeviceState, tools::HashEnum<DeviceTransition>> fkExpectedState;

  private:
    DDSSession fDDSSession;
    DDSTopology fDDSTopo;
    TopologyState fTopologyState;
    bool fStateChangeOngoing;
    DeviceState fTargetState;
    std::mutex fMtx;
    std::mutex fExecutionMtx;
    std::condition_variable fCV;
    std::condition_variable fExecutionCV;
    std::thread fExecutionThread;
    ChangeStateCallback fChangeStateCallback;
    std::chrono::milliseconds fStateChangeTimeout;
    bool fShutdown;

    void WaitForState();
    void AddNewStateEntry(uint64_t senderId, const std::string& state);
};

using Topo = Topology;

}   // namespace sdk
}   // namespace mq
}   // namespace fair

#endif /* FAIR_MQ_SDK_TOPOLOGY_H */
