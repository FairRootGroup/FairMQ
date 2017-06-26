/********************************************************************************
 *    Copyright (C) 2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_PLUGINSERVICES_H
#define FAIR_MQ_PLUGINSERVICES_H

#include <FairMQDevice.h>
#include <options/FairMQProgOptions.h>
#include <atomic>
#include <functional>
#include <string>
#include <unordered_map>

namespace fair
{
namespace mq
{

/**
 * @class PluginServices PluginServices.h <fairmq/PluginServices.h>
 * @brief Facilitates communication between devices and plugins
 *
 * - Configuration interface
 * - Control interface
 */
class PluginServices
{
    public:

    /// See https://github.com/FairRootGroup/FairRoot/blob/dev/fairmq/docs/Device.md#13-state-machine
    enum class DeviceState
    {
        Ok,
        Error,
        Idle,
        InitializingDevice,
        DeviceReady,
        InitializingTask,
        Ready,
        Running,
        Paused,
        ResettingTask,
        ResettingDevice,
        Exiting
    };
    enum class DeviceStateTransition  // transition event between DeviceStates
    {
        InitDevice,
        InitTask,
        Run,
        Pause,
        Stop,
        ResetTask,
        ResetDevice,
        End,
        ErrorFound
    };

    PluginServices() = delete;
    PluginServices(FairMQProgOptions& config, FairMQDevice& device)
    : fDevice{device}
    , fConfig{config}
    , fConfigEnabled{false}
    , fkDeviceStateStrMap{
        {"Ok",                 DeviceState::Ok},
        {"Error",              DeviceState::Error},
        {"Idle",               DeviceState::Idle},
        {"InitializingDevice", DeviceState::InitializingDevice},
        {"DeviceReady",        DeviceState::DeviceReady},
        {"InitializingTask",   DeviceState::InitializingTask},
        {"Ready",              DeviceState::Ready},
        {"Running",            DeviceState::Running},
        {"Paused",             DeviceState::Paused},
        {"ResettingTask",      DeviceState::ResettingTask},
        {"ResettingDevice",    DeviceState::ResettingDevice},
        {"Exiting",            DeviceState::Exiting}
      }
    , fkStrDeviceStateMap{
        {DeviceState::Ok,                 "Ok"},
        {DeviceState::Error,              "Error"},
        {DeviceState::Idle,               "Idle"},
        {DeviceState::InitializingDevice, "InitializingDevice"},
        {DeviceState::DeviceReady,        "DeviceReady"},
        {DeviceState::InitializingTask,   "InitializingTask"},
        {DeviceState::Ready,              "Ready"},
        {DeviceState::Running,            "Running"},
        {DeviceState::Paused,             "Paused"},
        {DeviceState::ResettingTask,      "ResettingTask"},
        {DeviceState::ResettingDevice,    "ResettingDevice"},
        {DeviceState::Exiting,            "Exiting"}
      }
    , fkDeviceStateMap{
        {FairMQDevice::OK,                  DeviceState::Ok},
        {FairMQDevice::ERROR,               DeviceState::Error},
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
      }
    , fkDeviceStateTransitionMap{
        {DeviceStateTransition::InitDevice,  FairMQDevice::INIT_DEVICE},
        {DeviceStateTransition::InitTask,    FairMQDevice::INIT_TASK},
        {DeviceStateTransition::Run,         FairMQDevice::RUN},
        {DeviceStateTransition::Pause,       FairMQDevice::PAUSE},
        {DeviceStateTransition::Stop,        FairMQDevice::STOP},
        {DeviceStateTransition::ResetTask,   FairMQDevice::RESET_TASK},
        {DeviceStateTransition::ResetDevice, FairMQDevice::RESET_DEVICE},
        {DeviceStateTransition::End,         FairMQDevice::END},
        {DeviceStateTransition::ErrorFound,  FairMQDevice::ERROR_FOUND}
      }
    {
    }

    // Control

    /// @brief Convert string to DeviceState
    /// @param state to convert
    /// @return DeviceState enum entry 
    /// @throw std::out_of_range if a string cannot be resolved to a DeviceState
    auto ToDeviceState(const std::string& state) const -> DeviceState
    {
        return fkDeviceStateStrMap.at(state);
    }

    /// @brief Convert DeviceState to string
    /// @param string to convert
    /// @return string representation of DeviceState enum entry 
    auto ToStr(DeviceState state) const -> std::string
    {
        return fkStrDeviceStateMap.at(state);
    }

    /// @return current device state
    auto GetCurrentDeviceState() const -> DeviceState
    {
        return fkDeviceStateMap.at(static_cast<FairMQDevice::State>(fDevice.GetCurrentState()));
    }

    /// @brief Trigger a device state transition
    /// @param next state transition
    ///
    /// The state transition may not happen immediately, but when the current state evaluates the
    /// pending transition event and terminates. In other words, the device states are scheduled cooperatively.
    auto ChangeDeviceState(const DeviceStateTransition next) -> void
    {
        fDevice.ChangeState(fkDeviceStateTransitionMap.at(next));
    }

    /// @brief Subscribe with a callback to device state changes
    /// @param InputMsgCallback
    ///
    /// The callback will be called at the beginning of a new state. The callback is called from the thread
    /// the state is running in.
    auto SubscribeToDeviceStateChange(const std::string& key, std::function<void(DeviceState /*newState*/)> callback) -> void
    {
        fDevice.OnStateChange(key, [&,callback](FairMQDevice::State newState){
            callback(fkDeviceStateMap.at(newState));
        });
    }

    auto UnsubscribeFromDeviceStateChange(const std::string& key) -> void
    {
        fDevice.UnsubscribeFromStateChange(key);
    }


    //// Configuration

    //// Writing only works during Initializing_device state
    //template<typename T>
    //auto SetProperty(const std::string& key, T val) -> void;

    //template<typename T>
    //auto GetProperty(const std::string& key) const -> T;
    //auto GetPropertyAsString(const std::string& key) const -> std::string;

    //template<typename T>
    //auto SubscribeToPropertyChange(
        //const std::string& key,
        //std::function<void(const std::string& [>key*/, const T /*newValue<])> callback
    //) const -> void;
    //auto UnsubscribeFromPropertyChange(const std::string& key) -> void;

    private:

    FairMQProgOptions& fConfig;
    FairMQDevice& fDevice;
    std::atomic<bool> fConfigEnabled;
    const std::unordered_map<std::string, DeviceState> fkDeviceStateStrMap;
    const std::unordered_map<DeviceState, std::string> fkStrDeviceStateMap;
    const std::unordered_map<FairMQDevice::State, DeviceState> fkDeviceStateMap;
    const std::unordered_map<DeviceStateTransition, FairMQDevice::Event> fkDeviceStateTransitionMap;
}; /* class PluginServices */

} /* namespace mq */
} /* namespace fair */

#endif /* FAIR_MQ_PLUGINSERVICES_H */
