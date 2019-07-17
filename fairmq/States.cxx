/********************************************************************************
 *    Copyright (C) 2019 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <fairmq/States.h>

#include <array>
#include <unordered_map>

using namespace std;

namespace fair
{
namespace mq
{

array<string, 15> stateNames =
{
    {
        "OK",
        "ERROR",
        "IDLE",
        "INITIALIZING DEVICE",
        "INITIALIZED",
        "BINDING",
        "BOUND",
        "CONNECTING",
        "DEVICE READY",
        "INITIALIZING TASK",
        "READY",
        "RUNNING",
        "RESETTING TASK",
        "RESETTING DEVICE",
        "EXITING"
    }
};

unordered_map<string, State> states =
{
    { "OK",                  State::Ok },
    { "ERROR",               State::Error },
    { "IDLE",                State::Idle },
    { "INITIALIZING DEVICE", State::InitializingDevice },
    { "INITIALIZED",         State::Initialized },
    { "BINDING",             State::Binding },
    { "BOUND",               State::Bound },
    { "CONNECTING",          State::Connecting },
    { "DEVICE READY",        State::DeviceReady },
    { "INITIALIZING TASK",   State::InitializingTask },
    { "READY",               State::Ready },
    { "RUNNING",             State::Running },
    { "RESETTING TASK",      State::ResettingTask },
    { "RESETTING DEVICE",    State::ResettingDevice },
    { "EXITING",             State::Exiting }
};

array<string, 12> transitionNames =
{
    {
        "AUTO",
        "INIT DEVICE",
        "COMPLETE INIT",
        "BIND",
        "CONNECT",
        "INIT TASK",
        "RUN",
        "STOP",
        "RESET TASK",
        "RESET DEVICE",
        "END",
        "ERROR FOUND"
    }
};

unordered_map<string, Transition> transitions =
{
    { "AUTO",          Transition::Auto },
    { "INIT DEVICE",   Transition::InitDevice },
    { "COMPLETE INIT", Transition::CompleteInit },
    { "BIND",          Transition::Bind },
    { "CONNECT",       Transition::Connect },
    { "INIT TASK",     Transition::InitTask },
    { "RUN",           Transition::Run },
    { "STOP",          Transition::Stop },
    { "RESET TASK",    Transition::ResetTask },
    { "RESET DEVICE",  Transition::ResetDevice },
    { "END",           Transition::End },
    { "ERROR FOUND",   Transition::ErrorFound }
};

string GetStateName(const State state)
{
    return stateNames.at(static_cast<int>(state));
}

string GetTransitionName(const Transition transition)
{
    return transitionNames.at(static_cast<int>(transition));
}

State GetState(const string& state)
{
    return states.at(state);
}

Transition GetTransition(const string& transition)
{
    return transitions.at(transition);
}

} // namespace mq
} // namespace fair
