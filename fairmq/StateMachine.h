/********************************************************************************
 *    Copyright (C) 2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_STATEMACHINE_H
#define FAIR_MQ_STATEMACHINE_H

#include <utility>
#include <FairMQLogger.h>
#include <fairmq/Tools.h>
#include <fairmq/EventManager.h>
#include <deque>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <unordered_map>

namespace fair
{
namespace mq
{

/**
 * @class StateMachine StateMachine.h <fairmq/StateMachine.h>
 * @brief Implements the state machine for FairMQ devices
 *
 * See https://github.com/FairRootGroup/FairRoot/blob/dev/fairmq/docs/Device.md#13-state-machine
 */
class StateMachine
{
  public:
    enum class State : int
    {
        Ok,
        Error,
        Idle,
        InitializingDevice,
        DeviceReady,
        InitializingTask,
        Ready,
        Running,
        ResettingTask,
        ResettingDevice,
        Exiting
    };

    enum class StateTransition : int // transition event between States
    {
        InitDevice,
        InitTask,
        Run,
        Stop,
        ResetTask,
        ResetDevice,
        End,
        ErrorFound,
        Automatic
    };

    /// @brief Convert string to State
    /// @param state to convert
    /// @return State enum entry
    /// @throw std::out_of_range if a string cannot be resolved to a State
    static auto ToState(const std::string& state) -> State { return fkStateStrMap.at(state); }

    /// @brief Convert string to StateTransition
    /// @param transition to convert
    /// @return StateTransition enum entry
    /// @throw std::out_of_range if a string cannot be resolved to a StateTransition
    static auto ToStateTransition(const std::string& transition) -> StateTransition { return fkStateTransitionStrMap.at(transition); }

    /// @brief Convert State to string
    /// @param state to convert
    /// @return string representation of State enum entry
    static auto ToStr(State state) -> std::string { return fkStrStateMap.at(state); }

    /// @brief Convert StateTransition to string
    /// @param transition to convert
    /// @return string representation of StateTransition enum entry
    static auto ToStr(StateTransition transition) -> std::string { return fkStrStateTransitionMap.at(transition); }

    friend auto operator<<(std::ostream& os, const State& state) -> std::ostream& { return os << ToStr(state); }
    friend auto operator<<(std::ostream& os, const StateTransition& transition) -> std::ostream& { return os << ToStr(transition); }

    StateMachine();

    struct IllegalTransition : std::runtime_error { using std::runtime_error::runtime_error; };

    struct StateChange : Event<State> {};
    struct StateQueued : Event<State> {};
    auto SubscribeToStateChange(const std::string& subscriber, std::function<void(typename StateChange::KeyType newState, State lastState)> callback) -> void { fCallbacks.Subscribe<StateChange, State>(subscriber, callback); }
    auto UnsubscribeFromStateChange(const std::string& subscriber) -> void { fCallbacks.Unsubscribe<StateChange, State>(subscriber); }
    auto SubscribeToStateQueued(const std::string& subscriber, std::function<void(typename StateQueued::KeyType newState, State lastState)> callback) -> void { fCallbacks.Subscribe<StateQueued, State>(subscriber, callback); }
    auto UnsubscribeFromStateQueued(const std::string& subscriber) -> void { fCallbacks.Unsubscribe<StateQueued, State>(subscriber); }

    auto GetCurrentState() const -> State { std::lock_guard<std::mutex> lock{fMutex}; return fState; }
    auto GetCurrentErrorState() const -> State { std::lock_guard<std::mutex> lock{fMutex}; return fErrorState; }
    auto GetLastQueuedState() const -> State { std::lock_guard<std::mutex> lock{fMutex}; return fNextStates.back(); }

    auto ChangeState(StateTransition transition) -> void;

    auto Run() -> void;
    auto Reset() -> void;

    auto NextStatePending() -> bool;

  private:
    State fState;
    State fErrorState;
    std::deque<State> fNextStates;
    EventManager fCallbacks;

    static const std::unordered_map<std::string, State> fkStateStrMap;
    static const std::unordered_map<State, std::string, tools::HashEnum<State>> fkStrStateMap;
    static const std::unordered_map<std::string, StateTransition> fkStateTransitionStrMap;
    static const std::unordered_map<StateTransition, std::string, tools::HashEnum<StateTransition>> fkStrStateTransitionMap;

    mutable std::mutex fMutex;
    std::condition_variable fNewState;

    static auto Transition(const State currentState, const StateTransition transition) -> State;
}; /* class StateMachine */

} /* namespace mq */
} /* namespace fair */

#endif /* FAIR_MQ_STATEMACHINE_H */
