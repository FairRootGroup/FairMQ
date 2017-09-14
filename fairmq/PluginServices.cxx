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
    {"Error",               DeviceState::Error},
    {"IDLE",                DeviceState::Idle},
    {"INITIALIZING DEVICE", DeviceState::InitializingDevice},
    {"DEVICE READY",        DeviceState::DeviceReady},
    {"INITIALIZING TASK",   DeviceState::InitializingTask},
    {"READY",               DeviceState::Ready},
    {"RUNNING",             DeviceState::Running},
    {"PAUSED",              DeviceState::Paused},
    {"RESETTING TASK",      DeviceState::ResettingTask},
    {"RESETTING DEVICE",    DeviceState::ResettingDevice},
    {"EXITING",             DeviceState::Exiting}
};
const std::unordered_map<PluginServices::DeviceState, std::string, tools::HashEnum<PluginServices::DeviceState>> PluginServices::fkStrDeviceStateMap = {
    {DeviceState::Ok,                 "OK"},
    {DeviceState::Error,              "Error"},
    {DeviceState::Idle,               "IDLE"},
    {DeviceState::InitializingDevice, "INITIALIZING DEVICE"},
    {DeviceState::DeviceReady,        "DEVICE READY"},
    {DeviceState::InitializingTask,   "INITIALIZING TASK"},
    {DeviceState::Ready,              "READY"},
    {DeviceState::Running,            "RUNNING"},
    {DeviceState::Paused,             "PAUSED"},
    {DeviceState::ResettingTask,      "RESETTING TASK"},
    {DeviceState::ResettingDevice,    "RESETTING DEVICE"},
    {DeviceState::Exiting,            "EXITING"}
};
const std::unordered_map<std::string, PluginServices::DeviceStateTransition> PluginServices::fkDeviceStateTransitionStrMap = {
    {"INIT DEVICE",  DeviceStateTransition::InitDevice},
    {"INIT TASK",    DeviceStateTransition::InitTask},
    {"RUN",          DeviceStateTransition::Run},
    {"PAUSE",        DeviceStateTransition::Pause},
    {"STOP",         DeviceStateTransition::Stop},
    {"RESET TASK",   DeviceStateTransition::ResetTask},
    {"RESET DEVICE", DeviceStateTransition::ResetDevice},
    {"END",          DeviceStateTransition::End},
    {"ERROR FOUND",  DeviceStateTransition::ErrorFound},
};
const std::unordered_map<PluginServices::DeviceStateTransition, std::string, tools::HashEnum<PluginServices::DeviceStateTransition>> PluginServices::fkStrDeviceStateTransitionMap = {
    {DeviceStateTransition::InitDevice,  "INIT DEVICE"},
    {DeviceStateTransition::InitTask,    "INIT TASK"},
    {DeviceStateTransition::Run,         "RUN"},
    {DeviceStateTransition::Pause,       "PAUSE"},
    {DeviceStateTransition::Stop,        "STOP"},
    {DeviceStateTransition::ResetTask,   "RESET TASK"},
    {DeviceStateTransition::ResetDevice, "RESET DEVICE"},
    {DeviceStateTransition::End,         "END"},
    {DeviceStateTransition::ErrorFound,  "ERROR FOUND"},
};
const std::unordered_map<FairMQDevice::State, PluginServices::DeviceState, fair::mq::tools::HashEnum<FairMQDevice::State>> PluginServices::fkDeviceStateMap = {
    {FairMQDevice::OK,                  DeviceState::Ok},
    {FairMQDevice::Error,               DeviceState::Error},
    {FairMQDevice::IDLE,                DeviceState::Idle},
    {FairMQDevice::INITIALIZING_DEVICE, DeviceState::InitializingDevice},
    {FairMQDevice::DEVICE_READY,        DeviceState::DeviceReady},
    {FairMQDevice::INITIALIZING_TASK,   DeviceState::InitializingTask},
    {FairMQDevice::READY,               DeviceState::Ready},
    {FairMQDevice::RUNNING,             DeviceState::Running},
    {FairMQDevice::PAUSED,              DeviceState::Paused},
    {FairMQDevice::RESETTING_TASK,      DeviceState::ResettingTask},
    {FairMQDevice::RESETTING_DEVICE,    DeviceState::ResettingDevice},
    {FairMQDevice::EXITING,             DeviceState::Exiting}
};
const std::unordered_map<PluginServices::DeviceStateTransition, FairMQDevice::Event, tools::HashEnum<PluginServices::DeviceStateTransition>> PluginServices::fkDeviceStateTransitionMap = {
    {DeviceStateTransition::InitDevice,  FairMQDevice::INIT_DEVICE},
    {DeviceStateTransition::InitTask,    FairMQDevice::INIT_TASK},
    {DeviceStateTransition::Run,         FairMQDevice::RUN},
    {DeviceStateTransition::Pause,       FairMQDevice::PAUSE},
    {DeviceStateTransition::Stop,        FairMQDevice::STOP},
    {DeviceStateTransition::ResetTask,   FairMQDevice::RESET_TASK},
    {DeviceStateTransition::ResetDevice, FairMQDevice::RESET_DEVICE},
    {DeviceStateTransition::End,         FairMQDevice::END},
    {DeviceStateTransition::ErrorFound,  FairMQDevice::ERROR_FOUND}
};

auto PluginServices::ChangeDeviceState(const std::string& controller, const DeviceStateTransition next) -> void
{
    // lock_guard<mutex> lock{fDeviceControllerMutex};
    //
    // if (!fDeviceController) fDeviceController = controller;

    if (fDeviceController == controller)
    {
        fDevice->ChangeState(fkDeviceStateTransitionMap.at(next));
    }
    else
    {
        throw DeviceControlError{tools::ToString(
            "Plugin ", controller, " is not allowed to change device states. ",
            "Currently, plugin ", fDeviceController, " has taken control."
        )};
    }
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
            "Plugin ", controller, " is not allowed to take over control. ",
            "Currently, plugin ", fDeviceController, " has taken control."
        )};
    }

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
            throw DeviceControlError{tools::ToString("Plugin ", controller, " cannot release control because it has not taken over control.")};
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
