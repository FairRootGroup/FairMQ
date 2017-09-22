/********************************************************************************
 *    Copyright (C) 2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "StateMachine.h"

using namespace fair::mq;
using namespace std;

const std::unordered_map<std::string, StateMachine::State> StateMachine::fkStateStrMap = {
    {"OK",                  State::Ok},
    {"ERROR",               State::Error},
    {"IDLE",                State::Idle},
    {"INITIALIZING DEVICE", State::InitializingDevice},
    {"DEVICE READY",        State::DeviceReady},
    {"INITIALIZING TASK",   State::InitializingTask},
    {"READY",               State::Ready},
    {"RUNNING",             State::Running},
    {"RESETTING TASK",      State::ResettingTask},
    {"RESETTING DEVICE",    State::ResettingDevice},
    {"EXITING",             State::Exiting}
};
const std::unordered_map<StateMachine::State, std::string, tools::HashEnum<StateMachine::State>> StateMachine::fkStrStateMap = {
    {State::Ok,                 "OK"},
    {State::Error,              "ERROR"},
    {State::Idle,               "IDLE"},
    {State::InitializingDevice, "INITIALIZING DEVICE"},
    {State::DeviceReady,        "DEVICE READY"},
    {State::InitializingTask,   "INITIALIZING TASK"},
    {State::Ready,              "READY"},
    {State::Running,            "RUNNING"},
    {State::ResettingTask,      "RESETTING TASK"},
    {State::ResettingDevice,    "RESETTING DEVICE"},
    {State::Exiting,            "EXITING"}
};
const std::unordered_map<std::string, StateMachine::StateTransition> StateMachine::fkStateTransitionStrMap = {
    {"INIT DEVICE",  StateTransition::InitDevice},
    {"INIT TASK",    StateTransition::InitTask},
    {"RUN",          StateTransition::Run},
    {"STOP",         StateTransition::Stop},
    {"RESET TASK",   StateTransition::ResetTask},
    {"RESET DEVICE", StateTransition::ResetDevice},
    {"END",          StateTransition::End},
    {"ERROR FOUND",  StateTransition::ErrorFound},
    {"AUTOMATIC",    StateTransition::Automatic},
};
const std::unordered_map<StateMachine::StateTransition, std::string, tools::HashEnum<StateMachine::StateTransition>> StateMachine::fkStrStateTransitionMap = {
    {StateTransition::InitDevice,  "INIT DEVICE"},
    {StateTransition::InitTask,    "INIT TASK"},
    {StateTransition::Run,         "RUN"},
    {StateTransition::Stop,        "STOP"},
    {StateTransition::ResetTask,   "RESET TASK"},
    {StateTransition::ResetDevice, "RESET DEVICE"},
    {StateTransition::End,         "END"},
    {StateTransition::ErrorFound,  "ERROR FOUND"},
    {StateTransition::Automatic,   "AUTOMATIC"},
};

auto StateMachine::Run() -> void
{
    LOG(STATE) << "Starting FairMQ state machine";

    LOG(DEBUG) << "Entering initial " << fErrorState << " state (orthogonal error state machine)";
    LOG(STATE) << "Entering initial " << fState << " state";

    std::unique_lock<std::mutex> lock{fMutex};
    while (true)
    {
        while (fNextStates.empty())
        {
            fNewState.wait(lock);
        }

        State lastState;

        if (fNextStates.front() == State::Error)
        {
            // advance error FSM
            lastState = fErrorState;
            fErrorState = fNextStates.front();
            fNextStates.pop_front();
            LOG(ERROR) << "Entering " << fErrorState << " state (orthogonal error state machine)";
        }
        else
        {
            // advance regular FSM
            lastState = fState;
            fState = fNextStates.front();
            fNextStates.pop_front();
            LOG(STATE) << "Entering " << fState << " state";
        }
        lock.unlock();

        fCallbacks.Emit<StateChange, State>(fState, lastState);

        lock.lock();
        if (fState == State::Exiting || fErrorState == State::Error) break;
    }

    LOG(STATE) << "Exiting FairMQ state machine";
}

auto StateMachine::ChangeState(StateTransition transition) -> void
{
    State lastState;

    std::unique_lock<std::mutex> lock{fMutex};

    if (transition == StateTransition::ErrorFound)
    {
        lastState = fErrorState;
    }
    else if (fNextStates.empty())
    {
        lastState = fState;
    }
    else
    {
        lastState = fNextStates.back();
    }

    const State nextState{Transition(lastState, transition)};
    fNextStates.push_back(nextState);
    lock.unlock();

    fCallbacks.Emit<StateQueued, State>(nextState, lastState);
    fNewState.notify_one();
}
    
auto StateMachine::Transition(const State currentState, const StateTransition transition) -> State
{
    switch (currentState) {
        case State::Idle:
            if (transition == StateTransition::InitDevice ) return State::InitializingDevice;
            if (transition == StateTransition::End        ) return State::Exiting;
            break;
        case State::InitializingDevice:
            if (transition == StateTransition::Automatic  ) return State::DeviceReady;
            break;
        case State::DeviceReady:
            if (transition == StateTransition::InitTask   ) return State::InitializingTask;
            if (transition == StateTransition::ResetDevice) return State::ResettingDevice;
            break;
        case State::InitializingTask:
            if (transition == StateTransition::Automatic  ) return State::Ready;
            break;
        case State::Ready:
            if (transition == StateTransition::Run        ) return State::Running;
            if (transition == StateTransition::ResetTask  ) return State::ResettingTask;
            break;
        case State::Running:
            if (transition == StateTransition::Stop       ) return State::Ready;
            break;
        case State::ResettingTask:
            if (transition == StateTransition::Automatic  ) return State::DeviceReady;
            break;
        case State::ResettingDevice:
            if (transition == StateTransition::Automatic  ) return State::Idle;
            break;
        case State::Exiting:
            break;
        case State::Ok:
            if (transition == StateTransition::ErrorFound ) return State::Error;
            break;
        case State::Error:
            break;
    }
    throw IllegalTransition{tools::ToString("No transition ", transition, " from state ", currentState, ".")};
}

StateMachine::StateMachine()
: fState{State::Idle}
, fErrorState{State::Ok}
{
}

auto StateMachine::Reset() -> void
{
    std::unique_lock<std::mutex> lock{fMutex};

    fState = State::Idle;
    fErrorState = State::Ok;
    fNextStates.clear();
}

auto StateMachine::NextStatePending() -> bool
{
    std::unique_lock<std::mutex> lock{fMutex};

    return fNextStates.size() > 0;
}
