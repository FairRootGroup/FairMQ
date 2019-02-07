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

const std::unordered_map<std::string, PluginServices::DeviceState> PluginServices::fkDeviceStateStrMap = {
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
const std::unordered_map<PluginServices::DeviceState, std::string, tools::HashEnum<PluginServices::DeviceState>> PluginServices::fkStrDeviceStateMap = {
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
const std::unordered_map<std::string, PluginServices::DeviceStateTransition> PluginServices::fkDeviceStateTransitionStrMap = {
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
const std::unordered_map<PluginServices::DeviceStateTransition, std::string, tools::HashEnum<PluginServices::DeviceStateTransition>> PluginServices::fkStrDeviceStateTransitionMap = {
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
const std::unordered_map<fair::mq::State, PluginServices::DeviceState, fair::mq::tools::HashEnum<fair::mq::State>> PluginServices::fkDeviceStateMap = {
    {fair::mq::State::Ok,                 DeviceState::Ok},
    {fair::mq::State::Error,              DeviceState::Error},
    {fair::mq::State::Idle,               DeviceState::Idle},
    {fair::mq::State::InitializingDevice, DeviceState::InitializingDevice},
    {fair::mq::State::Initialized,        DeviceState::Initialized},
    {fair::mq::State::Binding,            DeviceState::Binding},
    {fair::mq::State::Bound,              DeviceState::Bound},
    {fair::mq::State::Connecting,         DeviceState::Connecting},
    {fair::mq::State::DeviceReady,        DeviceState::DeviceReady},
    {fair::mq::State::InitializingTask,   DeviceState::InitializingTask},
    {fair::mq::State::Ready,              DeviceState::Ready},
    {fair::mq::State::Running,            DeviceState::Running},
    {fair::mq::State::ResettingTask,      DeviceState::ResettingTask},
    {fair::mq::State::ResettingDevice,    DeviceState::ResettingDevice},
    {fair::mq::State::Exiting,            DeviceState::Exiting}
};
const std::unordered_map<PluginServices::DeviceStateTransition, fair::mq::Transition, tools::HashEnum<PluginServices::DeviceStateTransition>> PluginServices::fkDeviceStateTransitionMap = {
    {DeviceStateTransition::Auto,         fair::mq::Transition::Auto},
    {DeviceStateTransition::InitDevice,   fair::mq::Transition::InitDevice},
    {DeviceStateTransition::CompleteInit, fair::mq::Transition::CompleteInit},
    {DeviceStateTransition::Bind,         fair::mq::Transition::Bind},
    {DeviceStateTransition::Connect,      fair::mq::Transition::Connect},
    {DeviceStateTransition::InitTask,     fair::mq::Transition::InitTask},
    {DeviceStateTransition::Run,          fair::mq::Transition::Run},
    {DeviceStateTransition::Stop,         fair::mq::Transition::Stop},
    {DeviceStateTransition::ResetTask,    fair::mq::Transition::ResetTask},
    {DeviceStateTransition::ResetDevice,  fair::mq::Transition::ResetDevice},
    {DeviceStateTransition::End,          fair::mq::Transition::End},
    {DeviceStateTransition::ErrorFound,   fair::mq::Transition::ErrorFound}
};

auto PluginServices::ChangeDeviceState(const std::string& controller, const DeviceStateTransition next) -> bool
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

auto PluginServices::TakeDeviceControl(const std::string& controller) -> void
{
    lock_guard<mutex> lock{fDeviceControllerMutex};

    if (!fDeviceController)
    {
        fDeviceController = controller;
    }
    else if (fDeviceController == controller)
    {
        // nothing to do
    }
    else
    {
        throw DeviceControlError{tools::ToString(
            "Plugin '", controller, "' is not allowed to take over control. ",
            "Currently, plugin '", *fDeviceController, "' has taken control."
        )};
    }
}

auto PluginServices::StealDeviceControl(const std::string& controller) -> void
{
    lock_guard<mutex> lock{fDeviceControllerMutex};

    fDeviceController = controller;
}

auto PluginServices::ReleaseDeviceControl(const std::string& controller) -> void
{
    {
        lock_guard<mutex> lock{fDeviceControllerMutex};

        if (fDeviceController == controller)
        {
            fDeviceController = boost::none;
        }
        else
        {
            throw DeviceControlError{tools::ToString("Plugin '", controller, "' cannot release control because it has not taken over control.")};
        }
    }

    fReleaseDeviceControlCondition.notify_one();
}

auto PluginServices::GetDeviceController() const -> boost::optional<std::string>
{
    lock_guard<mutex> lock{fDeviceControllerMutex};

    return fDeviceController;
}

auto PluginServices::WaitForReleaseDeviceControl() -> void
{
    unique_lock<mutex> lock{fDeviceControllerMutex};

    while (fDeviceController)
    {
        fReleaseDeviceControlCondition.wait(lock);
    }
}
