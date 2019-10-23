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

#include <fairlogger/Logger.h>

#include <asio/associated_executor.hpp>
#include <asio/async_result.hpp>
#include <asio/steady_timer.hpp>
#include <asio/system_executor.hpp>

#include <algorithm>
#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <ostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace fair {
namespace mq {
namespace sdk {

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

struct DeviceStatus
{
    bool initialized;
    DeviceState state;
    DDSTask::Id taskId;
    DDSCollection::Id collectionId;
};

using TopologyState = std::vector<DeviceStatus>;
using TopologyStateIndex = std::unordered_map<DDSTask::Id, int>; //  task id -> index in the data vector
using TopologyStateByTask = std::unordered_map<DDSTask::Id, DeviceStatus>;
using TopologyStateByCollection = std::unordered_map<DDSCollection::Id, std::vector<DeviceStatus>>;
using TopologyTransition = fair::mq::Transition;

inline DeviceState AggregateState(const TopologyState& topologyState)
{
    DeviceState first = topologyState.begin()->state;

    if (std::all_of(topologyState.cbegin(), topologyState.cend(), [&](TopologyState::value_type i) {
            return i.state == first;
        })) {
        return first;
    }

    throw MixedStateError("State is not uniform");
}

inline bool StateEqualsTo(const TopologyState& topologyState, DeviceState state)
{
    return AggregateState(topologyState) == state;
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
        , fChangeStateOp()
        , fChangeStateOpTimer(ex)
        , fChangeStateTarget(DeviceState::Idle)
    {
        makeTopologyState();

        std::string activeTopo(fDDSSession.RequestCommanderInfo().activeTopologyName);
        std::string givenTopo(fDDSTopo.GetName());
        if (activeTopo != givenTopo) {
            throw RuntimeError("Given topology ", givenTopo, " is not activated (active: ", activeTopo, ")");
        }

        using namespace fair::mq::sdk::cmd;
        fDDSSession.SubscribeToCommands([&](const std::string& msg, const std::string& /* condition */, DDSChannel::Id senderId) {
            Cmds inCmds;
            inCmds.Deserialize(msg);
            // LOG(debug) << "Received " << inCmds.Size() << " command(s) with total size of " << msg.length() << " bytes: ";
            for (const auto& cmd : inCmds) {
                // LOG(debug) << " > " << cmd->GetType();
                switch (cmd->GetType()) {
                    case Type::state_change: {
                        DDSTask::Id taskId(static_cast<StateChange&>(*cmd).GetTaskId());
                        fDDSSession.UpdateChannelToTaskAssociation(senderId, taskId);
                        if (static_cast<StateChange&>(*cmd).GetCurrentState() == DeviceState::Exiting) {
                            Cmds outCmds;
                            outCmds.Add<StateChangeExitingReceived>();
                            fDDSSession.SendCommand(outCmds.Serialize(), senderId);
                        }
                        UpdateStateEntry(taskId, static_cast<StateChange&>(*cmd).GetCurrentState());
                    }
                    break;
                    case Type::state_change_subscription:
                        if (static_cast<StateChangeSubscription&>(*cmd).GetResult() != Result::Ok) {
                            LOG(error) << "State change subscription failed for " << static_cast<StateChangeSubscription&>(*cmd).GetDeviceId();
                        }
                    break;
                    case Type::state_change_unsubscription:
                        if (static_cast<StateChangeUnsubscription&>(*cmd).GetResult() != Result::Ok) {
                            LOG(error) << "State change unsubscription failed for " << static_cast<StateChangeUnsubscription&>(*cmd).GetDeviceId();
                        }
                    break;
                    case Type::transition_status: {
                        if (static_cast<TransitionStatus&>(*cmd).GetResult() != Result::Ok) {
                            LOG(error) << "Transition failed for " << static_cast<TransitionStatus&>(*cmd).GetDeviceId();
                            std::lock_guard<std::mutex> lk(fMtx);
                            if (!fChangeStateOp.IsCompleted() && fStateData.at(fStateIndex.at(fDDSSession.GetTaskId(senderId))).state != fChangeStateTarget) {
                                fChangeStateOpTimer.cancel();
                                fChangeStateOp.Complete(MakeErrorCode(ErrorCode::DeviceChangeStateFailed), fStateData);
                            }
                        }
                    }
                    break;
                    default:
                        LOG(warn) << "Unexpected/unknown command received: " << cmd->GetType();
                        LOG(warn) << "Origin: " << senderId;
                    break;
                }
            }
        });

        fDDSSession.StartDDSService();
        LOG(debug) << "Subscribing to state change";
        Cmds cmds(make<SubscribeToStateChange>());
        fDDSSession.SendCommand(cmds.Serialize());
    }

    /// not copyable
    BasicTopology(const BasicTopology&) = delete;
    BasicTopology& operator=(const BasicTopology&) = delete;

    /// movable
    BasicTopology(BasicTopology&&) = default;
    BasicTopology& operator=(BasicTopology&&) = default;

    ~BasicTopology()
    {
        std::lock_guard<std::mutex> lk(fMtx);
        fDDSSession.UnsubscribeFromCommands();
        try {
            fChangeStateOp.Cancel(fStateData);
        } catch (...) {}
    }

    using Duration = std::chrono::milliseconds;
    using ChangeStateCompletionSignature = void(std::error_code, TopologyState);

    /// @brief Initiate state transition on all FairMQ devices in this topology
    /// @param transition FairMQ device state machine transition
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
    ///               case fair::mq::ErrorCode::OperationInProgress:
    ///                 // async operation already in progress
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
    ///           case fair::mq::ErrorCode::OperationInProgress:
    ///             // async operation already in progress
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
    ///           case fair::mq::ErrorCode::OperationInProgress:
    ///             // async operation already in progress
    ///           default:
    ///         }
    ///     }
    /// }
    /// @endcode
    template<typename CompletionToken>
    auto AsyncChangeState(TopologyTransition transition,
                          Duration timeout,
                          CompletionToken&& token)
    {
        return asio::async_initiate<CompletionToken, ChangeStateCompletionSignature>(
            [&](auto handler) {
                std::lock_guard<std::mutex> lk(fMtx);

                if (fChangeStateOp.IsCompleted()) {
                    fChangeStateOp = ChangeStateOp(AsioBase<Executor, Allocator>::GetExecutor(),
                                                   AsioBase<Executor, Allocator>::GetAllocator(),
                                                   std::move(handler));
                    fChangeStateTarget = expectedState.at(transition);
                    ResetTransitionedCount(fChangeStateTarget);
                    cmd::Cmds cmds(cmd::make<cmd::ChangeState>(transition));
                    fDDSSession.SendCommand(cmds.Serialize());
                    if (timeout > std::chrono::milliseconds(0)) {
                        fChangeStateOpTimer.expires_after(timeout);
                        fChangeStateOpTimer.async_wait([&](std::error_code ec) {
                            if (!ec) {
                                std::lock_guard<std::mutex> lk2(fMtx);
                                fChangeStateOp.Timeout(fStateData);
                            }
                        });
                    }
                } else {
                    // TODO refactor to hide boiler plate
                    auto ex2(asio::get_associated_executor(handler, AsioBase<Executor, Allocator>::GetExecutor()));
                    auto alloc2(asio::get_associated_allocator(handler, AsioBase<Executor, Allocator>::GetAllocator()));
                    auto state(GetCurrentStateUnsafe());

                    ex2.post(
                        [h = std::move(handler), s = std::move(state)]() mutable {
                            try {
                                h(MakeErrorCode(ErrorCode::OperationInProgress), s);
                            } catch (const std::exception& e) {
                                LOG(error) << "Uncaught exception in completion handler: " << e.what();
                            } catch (...) {
                                LOG(error) << "Unknown uncaught exception in completion handler.";
                            }
                        },
                        alloc2);
                }
            },
            token);
    }

    template<typename CompletionToken>
    auto AsyncChangeState(TopologyTransition transition, CompletionToken&& token)
    {
        return AsyncChangeState(transition, Duration(0), std::move(token));
    }

    /// @brief Perform state transition on all FairMQ devices in this topology
    /// @param transition FairMQ device state machine transition
    /// @param timeout Timeout in milliseconds, 0 means no timeout
    /// @tparam CompletionToken Asio completion token type
    /// @throws std::system_error
    auto ChangeState(TopologyTransition transition, Duration timeout = Duration(0))
        -> std::pair<std::error_code, TopologyState>
    {
        tools::SharedSemaphore blocker;
        std::error_code ec;
        TopologyState state;
        AsyncChangeState(
            transition, timeout, [&, blocker](std::error_code _ec, TopologyState _state) mutable {
                ec = _ec;
                state = _state;
                blocker.Signal();
            });
        blocker.Wait();
        return {ec, state};
    }

    /// @brief Returns the current state of the topology
    /// @return map of id : DeviceStatus (initialized, state)
    auto GetCurrentState() const -> TopologyState
    {
        std::lock_guard<std::mutex> lk(fMtx);
        return fStateData;
    }

    auto AggregateState() const -> DeviceState { return sdk::AggregateState(GetCurrentState()); }

    auto StateEqualsTo(DeviceState state) const -> bool { return sdk::StateEqualsTo(GetCurrentState(), state); }

  private:
    using TransitionedCount = unsigned int;

    DDSSession fDDSSession;
    DDSTopology fDDSTopo;
    TopologyState fStateData;
    TopologyStateIndex fStateIndex;
    mutable std::mutex fMtx;

    using ChangeStateOp = AsioAsyncOp<Executor, Allocator, ChangeStateCompletionSignature>;
    ChangeStateOp fChangeStateOp;
    asio::steady_timer fChangeStateOpTimer;
    DeviceState fChangeStateTarget;
    TransitionedCount fTransitionedCount;

    auto makeTopologyState() -> void
    {
        fStateData.reserve(fDDSTopo.GetTasks().size());

        int index = 0;

        for (const auto& task : fDDSTopo.GetTasks()) {
            fStateData.push_back(DeviceStatus{false, DeviceState::Ok, task.GetId(), task.GetCollectionId()});
            fStateIndex.emplace(task.GetId(), index);
            index++;
        }
    }

    auto UpdateStateEntry(const DDSTask::Id taskId, const DeviceState state) -> void
    {
        try {
            std::lock_guard<std::mutex> lk(fMtx);
            DeviceStatus& task = fStateData.at(fStateIndex.at(taskId));
            task.initialized = true;
            task.state = state;
            if (task.state == fChangeStateTarget) {
                ++fTransitionedCount;
            }
            // LOG(debug) << "Updated state entry: taskId=" << taskId << ", state=" << state;
            TryChangeStateCompletion();
        } catch (const std::exception& e) {
            LOG(error) << "Exception in UpdateStateEntry: " << e.what();
        }
    }

    /// precodition: fMtx is locked.
    auto TryChangeStateCompletion() -> void
    {
        if (!fChangeStateOp.IsCompleted() && fTransitionedCount == fStateData.size()) {
            fChangeStateOpTimer.cancel();
            fChangeStateOp.Complete(fStateData);
        }
    }

    /// precodition: fMtx is locked.
    auto ResetTransitionedCount(DeviceState targetState) -> void
    {
        fTransitionedCount = std::count_if(fStateIndex.cbegin(), fStateIndex.cend(), [=](const auto& s) {
            return fStateData.at(s.second).state == targetState;
        });
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
