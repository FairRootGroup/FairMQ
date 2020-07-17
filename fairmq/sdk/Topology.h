/********************************************************************************
 *    Copyright (C) 2019 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_SDK_TOPOLOGY_H
#define FAIR_MQ_SDK_TOPOLOGY_H

#include <fairmq/sdk/AsioAsyncOp.h>
#include <fairmq/sdk/AsioBase.h>
#include <fairmq/sdk/commands/Commands.h>
#include <fairmq/sdk/DDSCollection.h>
#include <fairmq/sdk/DDSInfo.h>
#include <fairmq/sdk/DDSSession.h>
#include <fairmq/sdk/DDSTask.h>
#include <fairmq/sdk/DDSTopology.h>
#include <fairmq/sdk/Error.h>
#include <fairmq/States.h>
#include <fairmq/tools/Semaphore.h>
#include <fairmq/tools/Unique.h>

#include <fairlogger/Logger.h>
#ifndef FAIR_LOG
#define FAIR_LOG LOG
#endif /* ifndef FAIR_LOG */

#include <asio/associated_executor.hpp>
#include <asio/async_result.hpp>
#include <asio/steady_timer.hpp>
#include <asio/system_executor.hpp>

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <ostream>
#include <set>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

namespace fair {
namespace mq {
namespace sdk {

using DeviceId = std::string;
using DeviceState = fair::mq::State;
using DeviceTransition = fair::mq::Transition;

const std::map<DeviceTransition, DeviceState> expectedState =
{
    { DeviceTransition::InitDevice,   DeviceState::InitializingDevice },
    { DeviceTransition::CompleteInit, DeviceState::Initialized },
    { DeviceTransition::Bind,         DeviceState::Bound },
    { DeviceTransition::Connect,      DeviceState::DeviceReady },
    { DeviceTransition::InitTask,     DeviceState::Ready },
    { DeviceTransition::Run,          DeviceState::Running },
    { DeviceTransition::Stop,         DeviceState::Ready },
    { DeviceTransition::ResetTask,    DeviceState::DeviceReady },
    { DeviceTransition::ResetDevice,  DeviceState::Idle },
    { DeviceTransition::End,          DeviceState::Exiting }
};

// mirrors DeviceState, but adds a "Mixed" state that represents a topology where devices are currently not in the same state.
enum class AggregatedTopologyState : int
{
    Undefined = static_cast<int>(fair::mq::State::Undefined),
    Ok = static_cast<int>(fair::mq::State::Ok),
    Error = static_cast<int>(fair::mq::State::Error),
    Idle = static_cast<int>(fair::mq::State::Idle),
    InitializingDevice = static_cast<int>(fair::mq::State::InitializingDevice),
    Initialized = static_cast<int>(fair::mq::State::Initialized),
    Binding = static_cast<int>(fair::mq::State::Binding),
    Bound = static_cast<int>(fair::mq::State::Bound),
    Connecting = static_cast<int>(fair::mq::State::Connecting),
    DeviceReady = static_cast<int>(fair::mq::State::DeviceReady),
    InitializingTask = static_cast<int>(fair::mq::State::InitializingTask),
    Ready = static_cast<int>(fair::mq::State::Ready),
    Running = static_cast<int>(fair::mq::State::Running),
    ResettingTask = static_cast<int>(fair::mq::State::ResettingTask),
    ResettingDevice = static_cast<int>(fair::mq::State::ResettingDevice),
    Exiting = static_cast<int>(fair::mq::State::Exiting),
    Mixed
};

inline auto operator==(DeviceState lhs, AggregatedTopologyState rhs) -> bool
{
    return static_cast<int>(lhs) == static_cast<int>(rhs);
}

inline auto operator==(AggregatedTopologyState lhs, DeviceState rhs) -> bool
{
    return static_cast<int>(lhs) == static_cast<int>(rhs);
}

inline std::ostream& operator<<(std::ostream& os, const AggregatedTopologyState& state)
{
    if (state == AggregatedTopologyState::Mixed) {
        return os << "MIXED";
    } else {
        return os << static_cast<DeviceState>(state);
    }
}

inline std::string GetAggregatedTopologyStateName(AggregatedTopologyState s)
{
    if (s == AggregatedTopologyState::Mixed) {
        return "MIXED";
    } else {
        return GetStateName(static_cast<State>(s));
    }
}

inline AggregatedTopologyState GetAggregatedTopologyState(const std::string& state)
{
    if (state == "MIXED") {
        return AggregatedTopologyState::Mixed;
    } else {
        return static_cast<AggregatedTopologyState>(GetState(state));
    }
}

struct DeviceStatus
{
    bool subscribed_to_state_changes;
    DeviceState lastState;
    DeviceState state;
    DDSTask::Id taskId;
    DDSCollection::Id collectionId;
};

using DeviceProperty = std::pair<std::string, std::string>; /// pair := (key, value)
using DeviceProperties = std::vector<DeviceProperty>;
using DevicePropertyQuery = std::string; /// Boost regex supported
using FailedDevices = std::set<DeviceId>;

struct GetPropertiesResult
{
    struct Device
    {
        DeviceProperties props;
    };
    std::unordered_map<DeviceId, Device> devices;
    FailedDevices failed;
};

using TopologyState = std::vector<DeviceStatus>;
using TopologyStateIndex = std::unordered_map<DDSTask::Id, int>; //  task id -> index in the data vector
using TopologyStateByTask = std::unordered_map<DDSTask::Id, DeviceStatus>;
using TopologyStateByCollection = std::unordered_map<DDSCollection::Id, std::vector<DeviceStatus>>;
using TopologyTransition = fair::mq::Transition;

inline AggregatedTopologyState AggregateState(const TopologyState& topologyState)
{
    DeviceState first = topologyState.begin()->state;

    if (std::all_of(topologyState.cbegin(), topologyState.cend(), [&](TopologyState::value_type i) {
            return i.state == first;
        })) {
        return static_cast<AggregatedTopologyState>(first);
    }

    return AggregatedTopologyState::Mixed;
}

inline bool StateEqualsTo(const TopologyState& topologyState, DeviceState state)
{
    return AggregateState(topologyState) == static_cast<AggregatedTopologyState>(state);
}

inline TopologyStateByCollection GroupByCollectionId(const TopologyState& topologyState)
{
    TopologyStateByCollection state;
    for (const auto& ds : topologyState) {
        if (ds.collectionId != 0) {
            state[ds.collectionId].push_back(ds);
        }
    }

    return state;
}

inline TopologyStateByTask GroupByTaskId(const TopologyState& topologyState)
{
    TopologyStateByTask state;
    for (const auto& ds : topologyState) {
        state[ds.taskId] = ds;
    }

    return state;
}

/**
 * @class BasicTopology Topology.h <fairmq/sdk/Topology.h>
 * @tparam Executor Associated I/O executor
 * @tparam Allocator Associated default allocator
 * @brief Represents a FairMQ topology
 *
 * @par Thread Safety
 * @e Distinct @e objects: Safe.@n
 * @e Shared @e objects: Safe.
 */
template <typename Executor, typename Allocator>
class BasicTopology : public AsioBase<Executor, Allocator>
{
  public:
    /// @brief (Re)Construct a FairMQ topology from an existing DDS topology
    /// @param topo DDSTopology
    /// @param session DDSSession
    BasicTopology(DDSTopology topo, DDSSession session)
        : BasicTopology<Executor, Allocator>(asio::system_executor(), std::move(topo), std::move(session))
    {}

    /// @brief (Re)Construct a FairMQ topology from an existing DDS topology
    /// @param ex I/O executor to be associated
    /// @param topo DDSTopology
    /// @param session DDSSession
    /// @throws RuntimeError
    BasicTopology(const Executor& ex,
                  DDSTopology topo,
                  DDSSession session,
                  Allocator alloc = DefaultAllocator())
        : AsioBase<Executor, Allocator>(ex, std::move(alloc))
        , fDDSSession(std::move(session))
        , fDDSTopo(std::move(topo))
        , fStateData()
        , fStateIndex()
        , fHeartbeatsTimer(asio::system_executor())
        , fHeartbeatInterval(600000)
    {
        makeTopologyState();

        std::string activeTopo(fDDSSession.RequestCommanderInfo().activeTopologyName);
        std::string givenTopo(fDDSTopo.GetName());
        if (activeTopo != givenTopo) {
            throw RuntimeError("Given topology ", givenTopo, " is not activated (active: ", activeTopo, ")");
        }

        SubscribeToCommands();

        fDDSSession.StartDDSService();
        SubscribeToStateChanges();
    }

    /// not copyable
    BasicTopology(const BasicTopology&) = delete;
    BasicTopology& operator=(const BasicTopology&) = delete;

    /// movable
    BasicTopology(BasicTopology&&) = default;
    BasicTopology& operator=(BasicTopology&&) = default;

    ~BasicTopology()
    {
        UnsubscribeFromStateChanges();

        std::lock_guard<std::mutex> lk(fMtx);
        fDDSSession.UnsubscribeFromCommands();
        try {
            for (auto& op : fChangeStateOps) {
                op.second.Complete(MakeErrorCode(ErrorCode::OperationCanceled));
            }
        } catch (...) {}
    }

    void SubscribeToStateChanges()
    {
        // FAIR_LOG(debug) << "Subscribing to state change";
        cmd::Cmds cmds(cmd::make<cmd::SubscribeToStateChange>(fHeartbeatInterval.count()));
        fDDSSession.SendCommand(cmds.Serialize());

        fHeartbeatsTimer.expires_after(fHeartbeatInterval);
        fHeartbeatsTimer.async_wait(std::bind(&BasicTopology::SendSubscriptionHeartbeats, this, std::placeholders::_1));
    }

    void SendSubscriptionHeartbeats(const std::error_code& ec)
    {
        if (!ec) {
            // Timer expired.
            fDDSSession.SendCommand(cmd::Cmds(cmd::make<cmd::SubscriptionHeartbeat>(fHeartbeatInterval.count())).Serialize());
            // schedule again
            fHeartbeatsTimer.expires_after(fHeartbeatInterval);
            fHeartbeatsTimer.async_wait(std::bind(&BasicTopology::SendSubscriptionHeartbeats, this, std::placeholders::_1));
        } else if (ec == asio::error::operation_aborted) {
            // FAIR_LOG(debug) << "Heartbeats timer canceled";
        } else {
            FAIR_LOG(error) << "Timer error: " << ec;
        }
    }

    void UnsubscribeFromStateChanges()
    {
        // stop sending heartbeats
        fHeartbeatsTimer.cancel();

        // unsubscribe from state changes
        fDDSSession.SendCommand(cmd::Cmds(cmd::make<cmd::UnsubscribeFromStateChange>()).Serialize());

        // wait for all tasks to confirm unsubscription
        std::unique_lock<std::mutex> lk(fMtx);
        fStateChangeUnsubscriptionCV.wait(lk, [&](){
            unsigned int count = std::count_if(fStateIndex.cbegin(), fStateIndex.cend(), [=](const auto& s) {
                return fStateData.at(s.second).subscribed_to_state_changes == false;
            });
            return count == fStateIndex.size();
        });
    }

    void SubscribeToCommands()
    {
        fDDSSession.SubscribeToCommands([&](const std::string& msg, const std::string& /* condition */, DDSChannel::Id senderId) {
            cmd::Cmds inCmds;
            inCmds.Deserialize(msg);
            // FAIR_LOG(debug) << "Received " << inCmds.Size() << " command(s) with total size of " << msg.length() << " bytes: ";

            for (const auto& cmd : inCmds) {
                // FAIR_LOG(debug) << " > " << cmd->GetType();
                switch (cmd->GetType()) {
                    case cmd::Type::state_change_subscription:
                        HandleCmd(static_cast<cmd::StateChangeSubscription&>(*cmd));
                    break;
                    case cmd::Type::state_change_unsubscription:
                        HandleCmd(static_cast<cmd::StateChangeUnsubscription&>(*cmd));
                    break;
                    case cmd::Type::state_change:
                        HandleCmd(static_cast<cmd::StateChange&>(*cmd), senderId);
                    break;
                    case cmd::Type::transition_status:
                        HandleCmd(static_cast<cmd::TransitionStatus&>(*cmd));
                    break;
                    case cmd::Type::properties:
                        HandleCmd(static_cast<cmd::Properties&>(*cmd));
                    break;
                    case cmd::Type::properties_set:
                        HandleCmd(static_cast<cmd::PropertiesSet&>(*cmd));
                    break;
                    default:
                        FAIR_LOG(warn) << "Unexpected/unknown command received: " << cmd->GetType();
                        FAIR_LOG(warn) << "Origin: " << senderId;
                    break;
                }
            }
        });
    }

    auto HandleCmd(cmd::StateChangeSubscription const& cmd) -> void
    {
        if (cmd.GetResult() == cmd::Result::Ok) {
            DDSTask::Id taskId(cmd.GetTaskId());

            try {
                std::lock_guard<std::mutex> lk(fMtx);
                DeviceStatus& task = fStateData.at(fStateIndex.at(taskId));
                task.subscribed_to_state_changes = true;
            } catch (const std::exception& e) {
                FAIR_LOG(error) << "Exception in HandleCmd(cmd::StateChangeSubscription const&): " << e.what();
            }
        } else {
            FAIR_LOG(error) << "State change subscription failed for device: " << cmd.GetDeviceId() << ", task id: " << cmd.GetTaskId();
        }
    }

    auto HandleCmd(cmd::StateChangeUnsubscription const& cmd) -> void
    {
        if (cmd.GetResult() == cmd::Result::Ok) {
            DDSTask::Id taskId(cmd.GetTaskId());

            try {
                std::unique_lock<std::mutex> lk(fMtx);
                DeviceStatus& task = fStateData.at(fStateIndex.at(taskId));
                task.subscribed_to_state_changes = false;
                lk.unlock();
                fStateChangeUnsubscriptionCV.notify_one();
            } catch (const std::exception& e) {
                FAIR_LOG(error) << "Exception in HandleCmd(cmd::StateChangeUnsubscription const&): " << e.what();
            }
        } else {
            FAIR_LOG(error) << "State change unsubscription failed for device: " << cmd.GetDeviceId() << ", task id: " << cmd.GetTaskId();
        }
    }

    auto HandleCmd(cmd::StateChange const& cmd, DDSChannel::Id const& senderId) -> void
    {
        if (cmd.GetCurrentState() == DeviceState::Exiting) {
            fDDSSession.SendCommand(cmd::Cmds(cmd::make<cmd::StateChangeExitingReceived>()).Serialize(), senderId);
        }

        DDSTask::Id taskId(cmd.GetTaskId());

        try {
            std::lock_guard<std::mutex> lk(fMtx);
            DeviceStatus& task = fStateData.at(fStateIndex.at(taskId));
            task.lastState = cmd.GetLastState();
            task.state = cmd.GetCurrentState();
            // if the task is exiting, it will not respond to unsubscription request anymore, set it to false now.
            if (task.state == DeviceState::Exiting) {
                task.subscribed_to_state_changes = false;
            }
            // FAIR_LOG(debug) << "Updated state entry: taskId=" << taskId << ", state=" << state;

            for (auto& op : fChangeStateOps) {
                op.second.Update(taskId, cmd.GetCurrentState());
            }
            for (auto& op : fWaitForStateOps) {
                op.second.Update(taskId, cmd.GetLastState(), cmd.GetCurrentState());
            }
        } catch (const std::exception& e) {
            FAIR_LOG(error) << "Exception in HandleCmd(cmd::StateChange const&): " << e.what();
        }
    }

    auto HandleCmd(cmd::TransitionStatus const& cmd) -> void
    {
        if (cmd.GetResult() != cmd::Result::Ok) {
            FAIR_LOG(error) << cmd.GetTransition() << " transition failed for " << cmd.GetDeviceId();
            DDSTask::Id taskId(cmd.GetTaskId());
            std::lock_guard<std::mutex> lk(fMtx);
            for (auto& op : fChangeStateOps) {
                if (!op.second.IsCompleted() && op.second.ContainsTask(taskId) &&
                    fStateData.at(fStateIndex.at(taskId)).state != op.second.GetTargetState()) {
                    op.second.Complete(MakeErrorCode(ErrorCode::DeviceChangeStateFailed));
                }
            }
        }
    }

    auto HandleCmd(cmd::Properties const& cmd) -> void
    {
        std::unique_lock<std::mutex> lk(fMtx);
        try {
            auto& op(fGetPropertiesOps.at(cmd.GetRequestId()));
            lk.unlock();
            op.Update(cmd.GetDeviceId(), cmd.GetResult(), cmd.GetProps());
        } catch (std::out_of_range& e) {
            FAIR_LOG(debug) << "GetProperties operation (request id: " << cmd.GetRequestId()
                            << ") not found (probably completed or timed out), "
                            << "discarding reply of device " << cmd.GetDeviceId();
        }
    }

    auto HandleCmd(cmd::PropertiesSet const& cmd) -> void
    {
        std::unique_lock<std::mutex> lk(fMtx);
        try {
            auto& op(fSetPropertiesOps.at(cmd.GetRequestId()));
            lk.unlock();
            op.Update(cmd.GetDeviceId(), cmd.GetResult());
        } catch (std::out_of_range& e) {
            FAIR_LOG(debug) << "SetProperties operation (request id: " << cmd.GetRequestId()
                            << ") not found (probably completed or timed out), "
                            << "discarding reply of device " << cmd.GetDeviceId();
        }
    }

    using Duration = std::chrono::milliseconds;
    using ChangeStateCompletionSignature = void(std::error_code, TopologyState);

  private:
    struct ChangeStateOp
    {
        using Id = std::size_t;
        using Count = unsigned int;

        template<typename Handler>
        ChangeStateOp(Id id,
                      const TopologyTransition transition,
                      std::vector<DDSTask> tasks,
                      TopologyState& stateData,
                      Duration timeout,
                      std::mutex& mutex,
                      Executor const & ex,
                      Allocator const & alloc,
                      Handler&& handler)
            : fId(id)
            , fOp(ex, alloc, std::move(handler))
            , fStateData(stateData)
            , fTimer(ex)
            , fCount(0)
            , fTasks(std::move(tasks))
            , fTargetState(expectedState.at(transition))
            , fMtx(mutex)
        {
            if (timeout > std::chrono::milliseconds(0)) {
                fTimer.expires_after(timeout);
                fTimer.async_wait([&](std::error_code ec) {
                    if (!ec) {
                        std::lock_guard<std::mutex> lk(fMtx);
                        fOp.Timeout(fStateData);
                    }
                });
            }
            if (fTasks.empty()) {
                FAIR_LOG(warn) << "ChangeState initiated on an empty set of tasks, check the path argument.";
            }
        }
        ChangeStateOp() = delete;
        ChangeStateOp(const ChangeStateOp&) = delete;
        ChangeStateOp& operator=(const ChangeStateOp&) = delete;
        ChangeStateOp(ChangeStateOp&&) = default;
        ChangeStateOp& operator=(ChangeStateOp&&) = default;
        ~ChangeStateOp() = default;

        /// precondition: fMtx is locked.
        auto ResetCount(const TopologyStateIndex& stateIndex, const TopologyState& stateData) -> void
        {
            fCount = std::count_if(stateIndex.cbegin(), stateIndex.cend(), [=](const auto& s) {
                if (ContainsTask(stateData.at(s.second).taskId)) {
                    return stateData.at(s.second).state == fTargetState;
                } else {
                    return false;
                }
            });
        }

        /// precondition: fMtx is locked.
        auto Update(const DDSTask::Id taskId, const DeviceState currentState) -> void
        {
            if (!fOp.IsCompleted() && ContainsTask(taskId)) {
                if (currentState == fTargetState) {
                    ++fCount;
                }
                TryCompletion();
            }
        }

        /// precondition: fMtx is locked.
        auto TryCompletion() -> void
        {
            if (!fOp.IsCompleted() && fCount == fTasks.size()) {
                Complete(std::error_code());
            }
        }

        /// precondition: fMtx is locked.
        auto Complete(std::error_code ec) -> void
        {
            fTimer.cancel();
            fOp.Complete(ec, fStateData);
        }

        /// precondition: fMtx is locked.
        auto ContainsTask(DDSTask::Id id) -> bool
        {
            auto it = std::find_if(fTasks.begin(), fTasks.end(), [id](const DDSTask& t) { return t.GetId() == id; });
            return it != fTasks.end();
        }

        bool IsCompleted() { return fOp.IsCompleted(); }

        auto GetTargetState() const -> DeviceState { return fTargetState; }

      private:
        Id const fId;
        AsioAsyncOp<Executor, Allocator, ChangeStateCompletionSignature> fOp;
        TopologyState& fStateData;
        asio::steady_timer fTimer;
        Count fCount;
        std::vector<DDSTask> fTasks;
        DeviceState fTargetState;
        std::mutex& fMtx;
    };

  public:
    /// @brief Initiate state transition on all FairMQ devices in this topology
    /// @param transition FairMQ device state machine transition
    /// @param path Select a subset of FairMQ devices in this topology, empty selects all
    /// @param timeout Timeout in milliseconds, 0 means no timeout
    /// @param token Asio completion token
    /// @tparam CompletionToken Asio completion token type
    /// @throws std::system_error
    ///
    /// @par Usage examples
    /// With lambda:
    /// @code
    /// topo.AsyncChangeState(
    ///     fair::mq::sdk::TopologyTransition::InitDevice,
    ///     std::chrono::milliseconds(500),
    ///     [](std::error_code ec, TopologyState state) {
    ///         if (!ec) {
    ///             // success
    ///          } else if (ec.category().name() == "fairmq") {
    ///             switch (static_cast<fair::mq::ErrorCode>(ec.value())) {
    ///               case fair::mq::ErrorCode::OperationTimeout:
    ///                 // async operation timed out
    ///               case fair::mq::ErrorCode::OperationCanceled:
    ///                 // async operation canceled
    ///               case fair::mq::ErrorCode::DeviceChangeStateFailed:
    ///                 // failed to change state of a fairmq device
    ///               default:
    ///             }
    ///         }
    ///     }
    /// );
    /// @endcode
    /// With future:
    /// @code
    /// auto fut = topo.AsyncChangeState(fair::mq::sdk::TopologyTransition::InitDevice,
    ///                                  std::chrono::milliseconds(500),
    ///                                  asio::use_future);
    /// try {
    ///     fair::mq::sdk::TopologyState state = fut.get();
    ///     // success
    /// } catch (const std::system_error& ex) {
    ///     auto ec(ex.code());
    ///     if (ec.category().name() == "fairmq") {
    ///         switch (static_cast<fair::mq::ErrorCode>(ec.value())) {
    ///           case fair::mq::ErrorCode::OperationTimeout:
    ///             // async operation timed out
    ///           case fair::mq::ErrorCode::OperationCanceled:
    ///             // async operation canceled
    ///           case fair::mq::ErrorCode::DeviceChangeStateFailed:
    ///             // failed to change state of a fairmq device
    ///           default:
    ///         }
    ///     }
    /// }
    /// @endcode
    /// With coroutine (C++20, see https://en.cppreference.com/w/cpp/language/coroutines):
    /// @code
    /// try {
    ///     fair::mq::sdk::TopologyState state = co_await
    ///         topo.AsyncChangeState(fair::mq::sdk::TopologyTransition::InitDevice,
    ///                               std::chrono::milliseconds(500),
    ///                               asio::use_awaitable);
    ///     // success
    /// } catch (const std::system_error& ex) {
    ///     auto ec(ex.code());
    ///     if (ec.category().name() == "fairmq") {
    ///         switch (static_cast<fair::mq::ErrorCode>(ec.value())) {
    ///           case fair::mq::ErrorCode::OperationTimeout:
    ///             // async operation timed out
    ///           case fair::mq::ErrorCode::OperationCanceled:
    ///             // async operation canceled
    ///           case fair::mq::ErrorCode::DeviceChangeStateFailed:
    ///             // failed to change state of a fairmq device
    ///           default:
    ///         }
    ///     }
    /// }
    /// @endcode
    template<typename CompletionToken>
    auto AsyncChangeState(const TopologyTransition transition,
                          const std::string& path,
                          Duration timeout,
                          CompletionToken&& token)
    {
        return asio::async_initiate<CompletionToken, ChangeStateCompletionSignature>([&](auto handler) {
            typename ChangeStateOp::Id const id(tools::UuidHash());

            std::lock_guard<std::mutex> lk(fMtx);

            for (auto it = begin(fChangeStateOps); it != end(fChangeStateOps);) {
                if (it->second.IsCompleted()) {
                    it = fChangeStateOps.erase(it);
                } else {
                    ++it;
                }
            }

            auto p = fChangeStateOps.emplace(
                std::piecewise_construct,
                std::forward_as_tuple(id),
                std::forward_as_tuple(id,
                                      transition,
                                      fDDSTopo.GetTasks(path),
                                      fStateData,
                                      timeout,
                                      fMtx,
                                      AsioBase<Executor, Allocator>::GetExecutor(),
                                      AsioBase<Executor, Allocator>::GetAllocator(),
                                      std::move(handler)));

            cmd::Cmds cmds(cmd::make<cmd::ChangeState>(transition));
            fDDSSession.SendCommand(cmds.Serialize(), path);

            p.first->second.ResetCount(fStateIndex, fStateData);
            // TODO: make sure following operation properly queues the completion and not doing it directly out of initiation call.
            p.first->second.TryCompletion();

        },
        token);
    }

    /// @brief Initiate state transition on all FairMQ devices in this topology
    /// @param transition FairMQ device state machine transition
    /// @param token Asio completion token
    /// @tparam CompletionToken Asio completion token type
    /// @throws std::system_error
    template<typename CompletionToken>
    auto AsyncChangeState(const TopologyTransition transition, CompletionToken&& token)
    {
        return AsyncChangeState(transition, "", Duration(0), std::move(token));
    }

    /// @brief Initiate state transition on all FairMQ devices in this topology with a timeout
    /// @param transition FairMQ device state machine transition
    /// @param timeout Timeout in milliseconds, 0 means no timeout
    /// @param token Asio completion token
    /// @tparam CompletionToken Asio completion token type
    /// @throws std::system_error
    template<typename CompletionToken>
    auto AsyncChangeState(const TopologyTransition transition, Duration timeout, CompletionToken&& token)
    {
        return AsyncChangeState(transition, "", timeout, std::move(token));
    }

    /// @brief Initiate state transition on all FairMQ devices in this topology with a timeout
    /// @param transition FairMQ device state machine transition
    /// @param path Select a subset of FairMQ devices in this topology, empty selects all
    /// @param token Asio completion token
    /// @tparam CompletionToken Asio completion token type
    /// @throws std::system_error
    template<typename CompletionToken>
    auto AsyncChangeState(const TopologyTransition transition, const std::string& path, CompletionToken&& token)
    {
        return AsyncChangeState(transition, path, Duration(0), std::move(token));
    }

    /// @brief Perform state transition on FairMQ devices in this topology for a specified topology path
    /// @param transition FairMQ device state machine transition
    /// @param path Select a subset of FairMQ devices in this topology, empty selects all
    /// @param timeout Timeout in milliseconds, 0 means no timeout
    /// @throws std::system_error
    auto ChangeState(const TopologyTransition transition, const std::string& path = "", Duration timeout = Duration(0))
        -> std::pair<std::error_code, TopologyState>
    {
        tools::SharedSemaphore blocker;
        std::error_code ec;
        TopologyState state;
        AsyncChangeState(transition, path, timeout, [&, blocker](std::error_code _ec, TopologyState _state) mutable {
            ec = _ec;
            state = _state;
            blocker.Signal();
        });
        blocker.Wait();
        return {ec, state};
    }

    /// @brief Perform state transition on all FairMQ devices in this topology with a timeout
    /// @param transition FairMQ device state machine transition
    /// @param timeout Timeout in milliseconds, 0 means no timeout
    /// @throws std::system_error
    auto ChangeState(const TopologyTransition transition, Duration timeout)
        -> std::pair<std::error_code, TopologyState>
    {
        return ChangeState(transition, "", timeout);
    }

    /// @brief Returns the current state of the topology
    /// @return map of id : DeviceStatus
    auto GetCurrentState() const -> TopologyState
    {
        std::lock_guard<std::mutex> lk(fMtx);
        return fStateData;
    }

    auto AggregateState() const -> DeviceState { return sdk::AggregateState(GetCurrentState()); }

    auto StateEqualsTo(DeviceState state) const -> bool { return sdk::StateEqualsTo(GetCurrentState(), state); }

    using WaitForStateCompletionSignature = void(std::error_code);

  private:
    struct WaitForStateOp
    {
        using Id = std::size_t;
        using Count = unsigned int;

        template<typename Handler>
        WaitForStateOp(Id id,
                       DeviceState targetLastState,
                       DeviceState targetCurrentState,
                       std::vector<DDSTask> tasks,
                       Duration timeout,
                       std::mutex& mutex,
                       Executor const & ex,
                       Allocator const & alloc,
                       Handler&& handler)
            : fId(id)
            , fOp(ex, alloc, std::move(handler))
            , fTimer(ex)
            , fCount(0)
            , fTasks(std::move(tasks))
            , fTargetLastState(targetLastState)
            , fTargetCurrentState(targetCurrentState)
            , fMtx(mutex)
        {
            if (timeout > std::chrono::milliseconds(0)) {
                fTimer.expires_after(timeout);
                fTimer.async_wait([&](std::error_code ec) {
                    if (!ec) {
                        std::lock_guard<std::mutex> lk(fMtx);
                        fOp.Timeout();
                    }
                });
            }
            if (fTasks.empty()) {
                FAIR_LOG(warn) << "WaitForState initiated on an empty set of tasks, check the path argument.";
            }
        }
        WaitForStateOp() = delete;
        WaitForStateOp(const WaitForStateOp&) = delete;
        WaitForStateOp& operator=(const WaitForStateOp&) = delete;
        WaitForStateOp(WaitForStateOp&&) = default;
        WaitForStateOp& operator=(WaitForStateOp&&) = default;
        ~WaitForStateOp() = default;

        /// precondition: fMtx is locked.
        auto ResetCount(const TopologyStateIndex& stateIndex, const TopologyState& stateData) -> void
        {
            fCount = std::count_if(stateIndex.cbegin(), stateIndex.cend(), [=](const auto& s) {
                if (ContainsTask(stateData.at(s.second).taskId)) {
                    return stateData.at(s.second).state == fTargetCurrentState &&
                          (stateData.at(s.second).lastState == fTargetLastState || fTargetLastState == DeviceState::Undefined);
                } else {
                    return false;
                }
            });
        }

        /// precondition: fMtx is locked.
        auto Update(const DDSTask::Id taskId, const DeviceState lastState, const DeviceState currentState) -> void
        {
            if (!fOp.IsCompleted() && ContainsTask(taskId)) {
                if (currentState == fTargetCurrentState &&
                    (lastState == fTargetLastState || fTargetLastState == DeviceState::Undefined)) {
                    ++fCount;
                }
                TryCompletion();
            }
        }

        /// precondition: fMtx is locked.
        auto TryCompletion() -> void
        {
            if (!fOp.IsCompleted() && fCount == fTasks.size()) {
                fTimer.cancel();
                fOp.Complete();
            }
        }

        bool IsCompleted() { return fOp.IsCompleted(); }

      private:
        Id const fId;
        AsioAsyncOp<Executor, Allocator, WaitForStateCompletionSignature> fOp;
        asio::steady_timer fTimer;
        Count fCount;
        std::vector<DDSTask> fTasks;
        DeviceState fTargetLastState;
        DeviceState fTargetCurrentState;
        std::mutex& fMtx;

        /// precondition: fMtx is locked.
        auto ContainsTask(DDSTask::Id id) -> bool
        {
            auto it = std::find_if(fTasks.begin(), fTasks.end(), [id](const DDSTask& t) { return t.GetId() == id; });
            return it != fTasks.end();
        }
    };

  public:
    /// @brief Initiate waiting for selected FairMQ devices to reach given last & current state in this topology
    /// @param targetLastState the target last device state to wait for
    /// @param targetCurrentState the target device state to wait for
    /// @param path Select a subset of FairMQ devices in this topology, empty selects all
    /// @param timeout Timeout in milliseconds, 0 means no timeout
    /// @param token Asio completion token
    /// @tparam CompletionToken Asio completion token type
    /// @throws std::system_error
    template<typename CompletionToken>
    auto AsyncWaitForState(const DeviceState targetLastState,
                           const DeviceState targetCurrentState,
                           const std::string& path,
                           Duration timeout,
                           CompletionToken&& token)
    {
        return asio::async_initiate<CompletionToken, WaitForStateCompletionSignature>([&](auto handler) {
            typename GetPropertiesOp::Id const id(tools::UuidHash());

            std::lock_guard<std::mutex> lk(fMtx);

            for (auto it = begin(fWaitForStateOps); it != end(fWaitForStateOps);) {
                if (it->second.IsCompleted()) {
                    it = fWaitForStateOps.erase(it);
                } else {
                    ++it;
                }
            }

            auto p = fWaitForStateOps.emplace(
                std::piecewise_construct,
                std::forward_as_tuple(id),
                std::forward_as_tuple(id,
                                      targetLastState,
                                      targetCurrentState,
                                      fDDSTopo.GetTasks(path),
                                      timeout,
                                      fMtx,
                                      AsioBase<Executor, Allocator>::GetExecutor(),
                                      AsioBase<Executor, Allocator>::GetAllocator(),
                                      std::move(handler)));
            p.first->second.ResetCount(fStateIndex, fStateData);
            // TODO: make sure following operation properly queues the completion and not doing it directly out of initiation call.
            p.first->second.TryCompletion();
        },
        token);
    }

    /// @brief Initiate waiting for selected FairMQ devices to reach given last & current state in this topology
    /// @param targetLastState the target last device state to wait for
    /// @param targetCurrentState the target device state to wait for
    /// @param token Asio completion token
    /// @tparam CompletionToken Asio completion token type
    /// @throws std::system_error
    template<typename CompletionToken>
    auto AsyncWaitForState(const DeviceState targetLastState, const DeviceState targetCurrentState, CompletionToken&& token)
    {
        return AsyncWaitForState(targetLastState, targetCurrentState, "", Duration(0), std::move(token));
    }

    /// @brief Initiate waiting for selected FairMQ devices to reach given current state in this topology
    /// @param targetCurrentState the target device state to wait for
    /// @param token Asio completion token
    /// @tparam CompletionToken Asio completion token type
    /// @throws std::system_error
    template<typename CompletionToken>
    auto AsyncWaitForState(const DeviceState targetCurrentState, CompletionToken&& token)
    {
        return AsyncWaitForState(DeviceState::Undefined, targetCurrentState, "", Duration(0), std::move(token));
    }

    /// @brief Wait for selected FairMQ devices to reach given last & current state in this topology
    /// @param targetLastState the target last device state to wait for
    /// @param targetCurrentState the target device state to wait for
    /// @param path Select a subset of FairMQ devices in this topology, empty selects all
    /// @param timeout Timeout in milliseconds, 0 means no timeout
    /// @throws std::system_error
    auto WaitForState(const DeviceState targetLastState, const DeviceState targetCurrentState, const std::string& path = "", Duration timeout = Duration(0))
        -> std::error_code
    {
        tools::SharedSemaphore blocker;
        std::error_code ec;
        AsyncWaitForState(targetLastState, targetCurrentState, path, timeout, [&, blocker](std::error_code _ec) mutable {
            ec = _ec;
            blocker.Signal();
        });
        blocker.Wait();
        return ec;
    }

    /// @brief Wait for selected FairMQ devices to reach given current state in this topology
    /// @param targetCurrentState the target device state to wait for
    /// @param path Select a subset of FairMQ devices in this topology, empty selects all
    /// @param timeout Timeout in milliseconds, 0 means no timeout
    /// @throws std::system_error
    auto WaitForState(const DeviceState targetCurrentState, const std::string& path = "", Duration timeout = Duration(0))
        -> std::error_code
    {
        return WaitForState(DeviceState::Undefined, targetCurrentState, path, timeout);
    }

    using GetPropertiesCompletionSignature = void(std::error_code, GetPropertiesResult);

  private:
    struct GetPropertiesOp
    {
        using Id = std::size_t;
        using GetCount = unsigned int;

        template<typename Handler>
        GetPropertiesOp(Id id,
                        GetCount expectedCount,
                        Duration timeout,
                        std::mutex& mutex,
                        Executor const & ex,
                        Allocator const & alloc,
                        Handler&& handler)
            : fId(id)
            , fOp(ex, alloc, std::move(handler))
            , fTimer(ex)
            , fCount(0)
            , fExpectedCount(expectedCount)
            , fMtx(mutex)
        {
            if (timeout > std::chrono::milliseconds(0)) {
                fTimer.expires_after(timeout);
                fTimer.async_wait([&](std::error_code ec) {
                    if (!ec) {
                        std::lock_guard<std::mutex> lk(fMtx);
                        fOp.Timeout(fResult);
                    }
                });
            }
            if (expectedCount == 0) {
                FAIR_LOG(warn) << "GetProperties initiated on an empty set of tasks, check the path argument.";
            }
            // FAIR_LOG(debug) << "GetProperties " << fId << " with expected count of " << fExpectedCount << " started.";
        }
        GetPropertiesOp() = delete;
        GetPropertiesOp(const GetPropertiesOp&) = delete;
        GetPropertiesOp& operator=(const GetPropertiesOp&) = delete;
        GetPropertiesOp(GetPropertiesOp&&) = default;
        GetPropertiesOp& operator=(GetPropertiesOp&&) = default;
        ~GetPropertiesOp() = default;

        auto Update(const std::string& deviceId, cmd::Result result, DeviceProperties props) -> void
        {
            std::lock_guard<std::mutex> lk(fMtx);
            if (cmd::Result::Ok != result) {
                fResult.failed.insert(deviceId);
            } else {
                fResult.devices.insert({deviceId, {std::move(props)}});
            }
            ++fCount;
            TryCompletion();
        }

        bool IsCompleted() { return fOp.IsCompleted(); }

      private:
        Id const fId;
        AsioAsyncOp<Executor, Allocator, GetPropertiesCompletionSignature> fOp;
        asio::steady_timer fTimer;
        GetCount fCount;
        GetCount const fExpectedCount;
        GetPropertiesResult fResult;
        std::mutex& fMtx;

        /// precondition: fMtx is locked.
        auto TryCompletion() -> void
        {
            if (!fOp.IsCompleted() && fCount == fExpectedCount) {
                fTimer.cancel();
                if (fResult.failed.size() > 0) {
                    fOp.Complete(MakeErrorCode(ErrorCode::DeviceGetPropertiesFailed), std::move(fResult));
                } else {
                    fOp.Complete(std::move(fResult));
                }
            }
        }
    };

  public:
    /// @brief Initiate property query on selected FairMQ devices in this topology
    /// @param query Key(s) to be queried (regex)
    /// @param path Select a subset of FairMQ devices in this topology, empty selects all
    /// @param timeout Timeout in milliseconds, 0 means no timeout
    /// @param token Asio completion token
    /// @tparam CompletionToken Asio completion token type
    /// @throws std::system_error
    template<typename CompletionToken>
    auto AsyncGetProperties(DevicePropertyQuery const& query,
                            const std::string& path,
                            Duration timeout,
                            CompletionToken&& token)
    {
        return asio::async_initiate<CompletionToken, GetPropertiesCompletionSignature>(
            [&](auto handler) {
                typename GetPropertiesOp::Id const id(tools::UuidHash());

                std::lock_guard<std::mutex> lk(fMtx);

                for (auto it = begin(fGetPropertiesOps); it != end(fGetPropertiesOps);) {
                    if (it->second.IsCompleted()) {
                        it = fGetPropertiesOps.erase(it);
                    } else {
                        ++it;
                    }
                }

                fGetPropertiesOps.emplace(
                    std::piecewise_construct,
                    std::forward_as_tuple(id),
                    std::forward_as_tuple(id,
                                          fDDSTopo.GetTasks(path).size(),
                                          timeout,
                                          fMtx,
                                          AsioBase<Executor, Allocator>::GetExecutor(),
                                          AsioBase<Executor, Allocator>::GetAllocator(),
                                          std::move(handler)));

                cmd::Cmds const cmds(cmd::make<cmd::GetProperties>(id, query));
                fDDSSession.SendCommand(cmds.Serialize(), path);
            },
            token);
    }

    /// @brief Initiate property query on selected FairMQ devices in this topology
    /// @param query Key(s) to be queried (regex)
    /// @param token Asio completion token
    /// @tparam CompletionToken Asio completion token type
    /// @throws std::system_error
    template<typename CompletionToken>
    auto AsyncGetProperties(DevicePropertyQuery const& query, CompletionToken&& token)
    {
        return AsyncGetProperties(query, "", Duration(0), std::move(token));
    }

    /// @brief Query properties on selected FairMQ devices in this topology
    /// @param query Key(s) to be queried (regex)
    /// @param path Select a subset of FairMQ devices in this topology, empty selects all
    /// @param timeout Timeout in milliseconds, 0 means no timeout
    /// @throws std::system_error
    auto GetProperties(DevicePropertyQuery const& query, const std::string& path = "", Duration timeout = Duration(0))
        -> std::pair<std::error_code, GetPropertiesResult>
    {
        tools::SharedSemaphore blocker;
        std::error_code ec;
        GetPropertiesResult result;
        AsyncGetProperties(query, path, timeout, [&, blocker](std::error_code _ec, GetPropertiesResult _result) mutable {
            ec = _ec;
            result = _result;
            blocker.Signal();
        });
        blocker.Wait();
        return {ec, result};
    }

    using SetPropertiesCompletionSignature = void(std::error_code, FailedDevices);

  private:
    struct SetPropertiesOp
    {
        using Id = std::size_t;
        using SetCount = unsigned int;

        template<typename Handler>
        SetPropertiesOp(Id id,
                        SetCount expectedCount,
                        Duration timeout,
                        std::mutex& mutex,
                        Executor const & ex,
                        Allocator const & alloc,
                        Handler&& handler)
            : fId(id)
            , fOp(ex, alloc, std::move(handler))
            , fTimer(ex)
            , fCount(0)
            , fExpectedCount(expectedCount)
            , fFailedDevices()
            , fMtx(mutex)
        {
            if (timeout > std::chrono::milliseconds(0)) {
                fTimer.expires_after(timeout);
                fTimer.async_wait([&](std::error_code ec) {
                    if (!ec) {
                        std::lock_guard<std::mutex> lk(fMtx);
                        fOp.Timeout(fFailedDevices);
                    }
                });
            }
            if (expectedCount == 0) {
                FAIR_LOG(warn) << "SetProperties initiated on an empty set of tasks, check the path argument.";
            }
            // FAIR_LOG(debug) << "SetProperties " << fId << " with expected count of " << fExpectedCount << " started.";
        }
        SetPropertiesOp() = delete;
        SetPropertiesOp(const SetPropertiesOp&) = delete;
        SetPropertiesOp& operator=(const SetPropertiesOp&) = delete;
        SetPropertiesOp(SetPropertiesOp&&) = default;
        SetPropertiesOp& operator=(SetPropertiesOp&&) = default;
        ~SetPropertiesOp() = default;

        auto Update(const std::string& deviceId, cmd::Result result) -> void
        {
            std::lock_guard<std::mutex> lk(fMtx);
            if (cmd::Result::Ok != result) {
                fFailedDevices.insert(deviceId);
            }
            ++fCount;
            TryCompletion();
        }

        bool IsCompleted() { return fOp.IsCompleted(); }

      private:
        Id const fId;
        AsioAsyncOp<Executor, Allocator, SetPropertiesCompletionSignature> fOp;
        asio::steady_timer fTimer;
        SetCount fCount;
        SetCount const fExpectedCount;
        FailedDevices fFailedDevices;
        std::mutex& fMtx;

        /// precondition: fMtx is locked.
        auto TryCompletion() -> void
        {
            if (!fOp.IsCompleted() && fCount == fExpectedCount) {
                fTimer.cancel();
                if (fFailedDevices.size() > 0) {
                    fOp.Complete(MakeErrorCode(ErrorCode::DeviceSetPropertiesFailed), fFailedDevices);
                } else {
                    fOp.Complete(fFailedDevices);
                }
            }
        }
    };

  public:
    /// @brief Initiate property update on selected FairMQ devices in this topology
    /// @param props Properties to set
    /// @param path Select a subset of FairMQ devices in this topology, empty selects all
    /// @param timeout Timeout in milliseconds, 0 means no timeout
    /// @param token Asio completion token
    /// @tparam CompletionToken Asio completion token type
    /// @throws std::system_error
    template<typename CompletionToken>
    auto AsyncSetProperties(const DeviceProperties& props,
                            const std::string& path,
                            Duration timeout,
                            CompletionToken&& token)
    {
        return asio::async_initiate<CompletionToken, SetPropertiesCompletionSignature>(
            [&](auto handler) {
                typename SetPropertiesOp::Id const id(tools::UuidHash());

                std::lock_guard<std::mutex> lk(fMtx);

                for (auto it = begin(fGetPropertiesOps); it != end(fGetPropertiesOps);) {
                    if (it->second.IsCompleted()) {
                        it = fGetPropertiesOps.erase(it);
                    } else {
                        ++it;
                    }
                }

                fSetPropertiesOps.emplace(
                    std::piecewise_construct,
                    std::forward_as_tuple(id),
                    std::forward_as_tuple(id,
                                          fDDSTopo.GetTasks(path).size(),
                                          timeout,
                                          fMtx,
                                          AsioBase<Executor, Allocator>::GetExecutor(),
                                          AsioBase<Executor, Allocator>::GetAllocator(),
                                          std::move(handler)));

                cmd::Cmds const cmds(cmd::make<cmd::SetProperties>(id, props));
                fDDSSession.SendCommand(cmds.Serialize(), path);
            },
            token);
    }

    /// @brief Initiate property update on selected FairMQ devices in this topology
    /// @param props Properties to set
    /// @param token Asio completion token
    /// @tparam CompletionToken Asio completion token type
    /// @throws std::system_error
    template<typename CompletionToken>
    auto AsyncSetProperties(DeviceProperties const & props, CompletionToken&& token)
    {
        return AsyncSetProperties(props, "", Duration(0), std::move(token));
    }

    /// @brief Set properties on selected FairMQ devices in this topology
    /// @param props Properties to set
    /// @param path Select a subset of FairMQ devices in this topology, empty selects all
    /// @param timeout Timeout in milliseconds, 0 means no timeout
    /// @throws std::system_error
    auto SetProperties(DeviceProperties const& properties, const std::string& path = "", Duration timeout = Duration(0))
        -> std::pair<std::error_code, FailedDevices>
    {
        tools::SharedSemaphore blocker;
        std::error_code ec;
        FailedDevices failed;
        AsyncSetProperties(properties, path, timeout, [&, blocker](std::error_code _ec, FailedDevices _failed) mutable {
            ec = _ec;
            failed = _failed;
            blocker.Signal();
        });
        blocker.Wait();
        return {ec, failed};
    }

    Duration GetHeartbeatInterval() const { return fHeartbeatInterval; }
    void SetHeartbeatInterval(Duration duration) { fHeartbeatInterval = duration; }

  private:
    using TransitionedCount = unsigned int;

    DDSSession fDDSSession;
    DDSTopology fDDSTopo;
    TopologyState fStateData;
    TopologyStateIndex fStateIndex;

    mutable std::mutex fMtx;

    std::condition_variable fStateChangeUnsubscriptionCV;
    asio::steady_timer fHeartbeatsTimer;
    Duration fHeartbeatInterval;

    std::unordered_map<typename ChangeStateOp::Id, ChangeStateOp> fChangeStateOps;
    std::unordered_map<typename WaitForStateOp::Id, WaitForStateOp> fWaitForStateOps;
    std::unordered_map<typename SetPropertiesOp::Id, SetPropertiesOp> fSetPropertiesOps;
    std::unordered_map<typename GetPropertiesOp::Id, GetPropertiesOp> fGetPropertiesOps;

    auto makeTopologyState() -> void
    {
        fStateData.reserve(fDDSTopo.GetTasks().size());

        int index = 0;

        for (const auto& task : fDDSTopo.GetTasks()) {
            fStateData.push_back(DeviceStatus{false, DeviceState::Undefined, DeviceState::Undefined, task.GetId(), task.GetCollectionId()});
            fStateIndex.emplace(task.GetId(), index);
            index++;
        }
    }

    /// precodition: fMtx is locked.
    auto GetCurrentStateUnsafe() const -> TopologyState
    {
        return fStateData;
    }
};

using Topology = BasicTopology<DefaultExecutor, DefaultAllocator>;
using Topo = Topology;

/// @brief Helper to (Re)Construct a FairMQ topology based on already existing native DDS API objects
/// @param nativeSession Existing and initialized CSession (either via create() or attach())
/// @param nativeTopo Existing CTopology that is activated on the given nativeSession
/// @param env Optional DDSEnv (needed primarily for unit testing)
auto MakeTopology(dds::topology_api::CTopology nativeTopo,
                  std::shared_ptr<dds::tools_api::CSession> nativeSession,
                  DDSEnv env = {}) -> Topology;

}   // namespace sdk
}   // namespace mq
}   // namespace fair

#endif /* FAIR_MQ_SDK_TOPOLOGY_H */
