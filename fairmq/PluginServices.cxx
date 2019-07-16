/********************************************************************************
 *    Copyright (C) 2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <fairmq/PluginServices.h>

using namespace fair::mq;
using namespace std;

const unordered_map<string, PluginServices::DeviceState> PluginServices::fkDeviceStateStrMap = {
    {"OK",                  DeviceState::Ok},
    {"ERROR",               DeviceState::Error},
    {"IDLE",                DeviceState::Idle},
    {"INITIALIZING DEVICE", DeviceState::InitializingDevice},
    {"INITIALIZED",         DeviceState::Initialized},
    {"BINDING",             DeviceState::Binding},
    {"BOUND",               DeviceState::Bound},
    {"CONNECTING",          DeviceState::Connecting},
    {"DEVICE READY",        DeviceState::DeviceReady},
    {"INITIALIZING TASK",   DeviceState::InitializingTask},
    {"READY",               DeviceState::Ready},
    {"RUNNING",             DeviceState::Running},
    {"RESETTING TASK",      DeviceState::ResettingTask},
    {"RESETTING DEVICE",    DeviceState::ResettingDevice},
    {"EXITING",             DeviceState::Exiting}
};
const unordered_map<PluginServices::DeviceState, string, tools::HashEnum<PluginServices::DeviceState>> PluginServices::fkStrDeviceStateMap = {
    {DeviceState::Ok,                 "OK"},
    {DeviceState::Error,              "ERROR"},
    {DeviceState::Idle,               "IDLE"},
    {DeviceState::InitializingDevice, "INITIALIZING DEVICE"},
    {DeviceState::Initialized,        "INITIALIZED"},
    {DeviceState::Binding,            "BINDING"},
    {DeviceState::Bound,              "BOUND"},
    {DeviceState::Connecting,         "CONNECTING"},
    {DeviceState::DeviceReady,        "DEVICE READY"},
    {DeviceState::InitializingTask,   "INITIALIZING TASK"},
    {DeviceState::Ready,              "READY"},
    {DeviceState::Running,            "RUNNING"},
    {DeviceState::ResettingTask,      "RESETTING TASK"},
    {DeviceState::ResettingDevice,    "RESETTING DEVICE"},
    {DeviceState::Exiting,            "EXITING"}
};
const unordered_map<string, PluginServices::DeviceStateTransition> PluginServices::fkDeviceStateTransitionStrMap = {
    {"AUTO",          DeviceStateTransition::Auto},
    {"INIT DEVICE",   DeviceStateTransition::InitDevice},
    {"COMPLETE INIT", DeviceStateTransition::CompleteInit},
    {"BIND",          DeviceStateTransition::Bind},
    {"CONNECT",       DeviceStateTransition::Connect},
    {"INIT TASK",     DeviceStateTransition::InitTask},
    {"RUN",           DeviceStateTransition::Run},
    {"STOP",          DeviceStateTransition::Stop},
    {"RESET TASK",    DeviceStateTransition::ResetTask},
    {"RESET DEVICE",  DeviceStateTransition::ResetDevice},
    {"END",           DeviceStateTransition::End},
    {"ERROR FOUND",   DeviceStateTransition::ErrorFound},
};
const unordered_map<PluginServices::DeviceStateTransition, string, tools::HashEnum<PluginServices::DeviceStateTransition>> PluginServices::fkStrDeviceStateTransitionMap = {
    {DeviceStateTransition::Auto,         "Auto"},
    {DeviceStateTransition::InitDevice,   "INIT DEVICE"},
    {DeviceStateTransition::CompleteInit, "COMPLETE INIT"},
    {DeviceStateTransition::Bind,         "BIND"},
    {DeviceStateTransition::Connect,      "CONNECT"},
    {DeviceStateTransition::InitTask,     "INIT TASK"},
    {DeviceStateTransition::Run,          "RUN"},
    {DeviceStateTransition::Stop,         "STOP"},
    {DeviceStateTransition::ResetTask,    "RESET TASK"},
    {DeviceStateTransition::ResetDevice,  "RESET DEVICE"},
    {DeviceStateTransition::End,          "END"},
    {DeviceStateTransition::ErrorFound,   "ERROR FOUND"},
};
const unordered_map<State, PluginServices::DeviceState, tools::HashEnum<State>> PluginServices::fkDeviceStateMap = {
    {State::Ok,                 DeviceState::Ok},
    {State::Error,              DeviceState::Error},
    {State::Idle,               DeviceState::Idle},
    {State::InitializingDevice, DeviceState::InitializingDevice},
    {State::Initialized,        DeviceState::Initialized},
    {State::Binding,            DeviceState::Binding},
    {State::Bound,              DeviceState::Bound},
    {State::Connecting,         DeviceState::Connecting},
    {State::DeviceReady,        DeviceState::DeviceReady},
    {State::InitializingTask,   DeviceState::InitializingTask},
    {State::Ready,              DeviceState::Ready},
    {State::Running,            DeviceState::Running},
    {State::ResettingTask,      DeviceState::ResettingTask},
    {State::ResettingDevice,    DeviceState::ResettingDevice},
    {State::Exiting,            DeviceState::Exiting}
};
const unordered_map<PluginServices::DeviceState, State> PluginServices::fkStateMap = {
    {DeviceState::Ok,                 State::Ok},
    {DeviceState::Error,              State::Error},
    {DeviceState::Idle,               State::Idle},
    {DeviceState::InitializingDevice, State::InitializingDevice},
    {DeviceState::Initialized,        State::Initialized},
    {DeviceState::Binding,            State::Binding},
    {DeviceState::Bound,              State::Bound},
    {DeviceState::Connecting,         State::Connecting},
    {DeviceState::DeviceReady,        State::DeviceReady},
    {DeviceState::InitializingTask,   State::InitializingTask},
    {DeviceState::Ready,              State::Ready},
    {DeviceState::Running,            State::Running},
    {DeviceState::ResettingTask,      State::ResettingTask},
    {DeviceState::ResettingDevice,    State::ResettingDevice},
    {DeviceState::Exiting,            State::Exiting}
};
const unordered_map<PluginServices::DeviceStateTransition, Transition, tools::HashEnum<PluginServices::DeviceStateTransition>> PluginServices::fkDeviceStateTransitionMap = {
    {DeviceStateTransition::Auto,         Transition::Auto},
    {DeviceStateTransition::InitDevice,   Transition::InitDevice},
    {DeviceStateTransition::CompleteInit, Transition::CompleteInit},
    {DeviceStateTransition::Bind,         Transition::Bind},
    {DeviceStateTransition::Connect,      Transition::Connect},
    {DeviceStateTransition::InitTask,     Transition::InitTask},
    {DeviceStateTransition::Run,          Transition::Run},
    {DeviceStateTransition::Stop,         Transition::Stop},
    {DeviceStateTransition::ResetTask,    Transition::ResetTask},
    {DeviceStateTransition::ResetDevice,  Transition::ResetDevice},
    {DeviceStateTransition::End,          Transition::End},
    {DeviceStateTransition::ErrorFound,   Transition::ErrorFound}
};

auto PluginServices::ChangeDeviceState(const string& controller, const DeviceStateTransition next) -> bool
{
    lock_guard<mutex> lock{fDeviceControllerMutex};

    if (!fDeviceController) fDeviceController = controller;

    bool result = false;

    if (fDeviceController == controller) {
        result = fDevice.ChangeState(fkDeviceStateTransitionMap.at(next));
    } else {
        throw DeviceControlError{tools::ToString(
            "Plugin '", controller, "' is not allowed to change device states. ",
            "Currently, plugin '", *fDeviceController, "' has taken control."
        )};
    }

    return result;
}

void PluginServices::TransitionDeviceStateTo(const std::string& controller, DeviceState state)
{
    lock_guard<mutex> lock{fDeviceControllerMutex};

    if (!fDeviceController) fDeviceController = controller;

    if (fDeviceController == controller) {
        fDevice.TransitionTo(fkStateMap.at(state));
    } else {
        throw DeviceControlError{tools::ToString(
            "Plugin '", controller, "' is not allowed to change device states. ",
            "Currently, plugin '", *fDeviceController, "' has taken control."
        )};
    }
}

auto PluginServices::TakeDeviceControl(const string& controller) -> void
{
    lock_guard<mutex> lock{fDeviceControllerMutex};

    if (!fDeviceController) {
        fDeviceController = controller;
    } else if (fDeviceController == controller) {
        // nothing to do
    } else {
        throw DeviceControlError{tools::ToString(
            "Plugin '", controller, "' is not allowed to take over control. ",
            "Currently, plugin '", *fDeviceController, "' has taken control."
        )};
    }
}

auto PluginServices::StealDeviceControl(const string& controller) -> void
{
    lock_guard<mutex> lock{fDeviceControllerMutex};

    fDeviceController = controller;
}

auto PluginServices::ReleaseDeviceControl(const string& controller) -> void
{
    {
        lock_guard<mutex> lock{fDeviceControllerMutex};

        if (fDeviceController == controller) {
            fDeviceController = boost::none;
        } else {
            throw DeviceControlError{tools::ToString("Plugin '", controller, "' cannot release control because it has not taken over control.")};
        }
    }

    fReleaseDeviceControlCondition.notify_one();
}

auto PluginServices::GetDeviceController() const -> boost::optional<string>
{
    lock_guard<mutex> lock{fDeviceControllerMutex};

    return fDeviceController;
}

auto PluginServices::WaitForReleaseDeviceControl() -> void
{
    unique_lock<mutex> lock{fDeviceControllerMutex};

    while (fDeviceController) {
        fReleaseDeviceControlCondition.wait(lock);
    }
}
