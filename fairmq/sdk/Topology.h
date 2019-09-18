/********************************************************************************
 *    Copyright (C) 2019 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_SDK_TOPOLOGY_H
#define FAIR_MQ_SDK_TOPOLOGY_H

#include <algorithm>
#include <asio/async_result.hpp>
#include <asio/associated_executor.hpp>
#include <asio/steady_timer.hpp>
#include <asio/system_executor.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <chrono>
#include <fairlogger/Logger.h>
#include <fairmq/States.h>
#include <fairmq/Tools.h>
#include <fairmq/sdk/AsioAsyncOp.h>
#include <fairmq/sdk/AsioBase.h>
#include <fairmq/sdk/DDSCollection.h>
#include <fairmq/sdk/DDSInfo.h>
#include <fairmq/sdk/DDSSession.h>
#include <fairmq/sdk/DDSTask.h>
#include <fairmq/sdk/DDSTopology.h>
#include <fairmq/sdk/Error.h>
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
using TopologyStateIndex = std::unordered_map<DDSTask::Id, size_t>; //  task id -> index in the data vector
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

        fDDSSession.SubscribeToCommands([&](const std::string& msg, const std::string& /* condition */, DDSChannel::Id senderId) {
            // LOG(debug) << "Received from " << senderId << ": " << msg;
            std::vector<std::string> parts;
            boost::algorithm::split(parts, msg, boost::algorithm::is_any_of(":,"));

            for (unsigned int i = 0; i < parts.size(); ++i) {
                boost::trim(parts.at(i));
            }

            if (parts[0] == "state-change") {
                DDSTask::Id taskId(std::stoull(parts[2]));
                fDDSSession.UpdateChannelToTaskAssociation(senderId, taskId);
                if (parts[3] == "IDLE->EXITING") {
                    fDDSSession.SendCommand("state-change-exiting-received", senderId);
                }
                UpdateStateEntry(taskId, parts[3]);
            } else if (parts[0] == "state-changes-subscription") {
                LOG(debug) << "Received from " << senderId << ": " << msg;
                if (parts[2] != "OK") {
                    LOG(error) << "state-changes-subscription failed with return code: " << parts[2];
                }
            } else if (parts[0] == "state-changes-unsubscription") {
                LOG(debug) << "Received from " << senderId << ": " << msg;
                if (parts[2] != "OK") {
                    LOG(error) << "state-changes-unsubscription failed with return code: " << parts[2];
                }
            } else if (parts[1] == "could not queue") {
                std::lock_guard<std::mutex> lk(fMtx);
                if (!fChangeStateOp.IsCompleted() && fStateData.at(fStateIndex.at(fDDSSession.GetTaskId(senderId))).state != fChangeStateTarget) {
                    fChangeStateOpTimer.cancel();
                    fChangeStateOp.Complete(MakeErrorCode(ErrorCode::DeviceChangeStateFailed), fStateData);
                }
            }
        });

        fDDSSession.StartDDSService();
        LOG(debug) << "subscribe-to-state-changes";
        fDDSSession.SendCommand("subscribe-to-state-changes");
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
                    fDDSSession.SendCommand(GetTransitionName(transition));
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
                    auto ex2(asio::get_associated_executor(
                        handler, AsioBase<Executor, Allocator>::GetExecutor()));
                    auto alloc2(asio::get_associated_allocator(
                        handler, AsioBase<Executor, Allocator>::GetAllocator()));
                    auto state(GetCurrentStateUnsafe());

                    ex2.post(
                        [h = std::move(handler), s = std::move(state)]() mutable {
                            try {
                                h(MakeErrorCode(ErrorCode::OperationInProgress), s);
                            } catch (const std::exception& e) {
                                LOG(error)
                                    << "Uncaught exception in completion handler: " << e.what();
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

        size_t index = 0;

        for (const auto& task : fDDSTopo.GetTasks()) {
            fStateData.push_back(DeviceStatus{false, DeviceState::Ok, task.GetId(), task.GetCollectionId()});
            fStateIndex.emplace(task.GetId(), index);
            index++;
        }
    }

    auto UpdateStateEntry(DDSTask::Id taskId, const std::string& state) -> void
    {
        std::size_t pos = state.find("->");
        std::string endState = state.substr(pos + 2);
        try {
            std::lock_guard<std::mutex> lk(fMtx);
            DeviceStatus& task = fStateData.at(fStateIndex.at(taskId));
            task.initialized = true;
            task.state = fair::mq::GetState(endState);
            if (task.state == fChangeStateTarget) {
                ++fTransitionedCount;
            }
            // LOG(debug) << "Updated state entry: taskId=" << taskId << ", state=" << endState;
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
