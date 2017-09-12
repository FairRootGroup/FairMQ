/********************************************************************************
 *    Copyright (C) 2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_PLUGINSERVICES_H
#define FAIR_MQ_PLUGINSERVICES_H

#include <fairmq/Tools.h>
#include <FairMQDevice.h>
#include <options/FairMQProgOptions.h>
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

    PluginServices() = delete;
    PluginServices(FairMQProgOptions* config, std::shared_ptr<FairMQDevice> device)
    : fDevice{device}
    , fConfig{config}
    {
    }

    /// See https://github.com/FairRootGroup/FairRoot/blob/dev/fairmq/docs/Device.md#13-state-machine
    enum class DeviceState : int
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

    enum class DeviceStateTransition : int // transition event between DeviceStates
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

    // Control API

    /// @brief Convert string to DeviceState
    /// @param state to convert
    /// @return DeviceState enum entry 
    /// @throw std::out_of_range if a string cannot be resolved to a DeviceState
    static auto ToDeviceState(const std::string& state) -> DeviceState { return fkDeviceStateStrMap.at(state); }

    /// @brief Convert string to DeviceStateTransition
    /// @param transition to convert
    /// @return DeviceStateTransition enum entry 
    /// @throw std::out_of_range if a string cannot be resolved to a DeviceStateTransition
    static auto ToDeviceStateTransition(const std::string& transition) -> DeviceStateTransition { return fkDeviceStateTransitionStrMap.at(transition); }

    /// @brief Convert DeviceState to string
    /// @param state to convert
    /// @return string representation of DeviceState enum entry 
    static auto ToStr(DeviceState state) -> std::string { return fkStrDeviceStateMap.at(state); }

    /// @brief Convert DeviceStateTransition to string
    /// @param transition to convert
    /// @return string representation of DeviceStateTransition enum entry 
    static auto ToStr(DeviceStateTransition transition) -> std::string { return fkStrDeviceStateTransitionMap.at(transition); }

    friend auto operator<<(std::ostream& os, const DeviceState& state) -> std::ostream& { return os << ToStr(state); }
    friend auto operator<<(std::ostream& os, const DeviceStateTransition& transition) -> std::ostream& { return os << ToStr(transition); }

    /// @return current device state
    auto GetCurrentDeviceState() const -> DeviceState { return fkDeviceStateMap.at(static_cast<FairMQDevice::State>(fDevice->GetCurrentState())); }

    /// @brief Request a device state transition
    /// @param next state transition
    ///
    /// The state transition may not happen immediately, but when the current state evaluates the
    /// pending transition event and terminates. In other words, the device states are scheduled cooperatively.
    auto ChangeDeviceState(const DeviceStateTransition next) -> void { fDevice->ChangeState(fkDeviceStateTransitionMap.at(next)); }

    /// @brief Subscribe with a callback to device state changes
    /// @param subscriber id
    /// @param callback
    ///
    /// The callback will be called at the beginning of a new state. The callback is called from the thread
    /// the state is running in.
    auto SubscribeToDeviceStateChange(const std::string& subscriber, std::function<void(DeviceState /*newState*/)> callback) -> void
    {
        fDevice->SubscribeToStateChange(subscriber, [&,callback](FairMQDevice::State newState){
            callback(fkDeviceStateMap.at(newState));
        });
    }

    auto TakeControl(const std::string& controller) -> void { };
    auto ReleaseControl(const std::string& controller) -> void { };

    /// @brief Unsubscribe from device state changes
    /// @param subscriber id
    auto UnsubscribeFromDeviceStateChange(const std::string& subscriber) -> void { fDevice->UnsubscribeFromStateChange(subscriber); }


    // Config API

    /// @brief Set config property
    /// @param key
    /// @param val
    /// @throws fair::mq::PluginServices::InvalidStateError if method is called in unsupported device states
    ///
    /// Setting a config property will store the value in the FairMQ internal config store and notify any subscribers about the update.
    /// It is property dependent, if the call to this method will have an immediate, delayed or any effect at all.
    template<typename T>
    auto SetProperty(const std::string& key, T val) -> void
    {
        auto currentState = GetCurrentDeviceState();
        if (currentState == DeviceState::InitializingDevice)
        {
            fConfig->SetValue(key, val);
        }
        else
        {
            throw InvalidStateError{tools::ToString("PluginServices::SetProperty is not supported in device state ", currentState, ". Supported state is ", DeviceState::InitializingDevice, ".")};
        }
    }
    struct InvalidStateError : std::runtime_error { using std::runtime_error::runtime_error; };

    /// @brief Read config property
    /// @param key
    /// @return config property value
    ///
    /// TODO Currently, if a non-existing key is requested and a default constructed object is returned.
    /// This behaviour will be changed in the future to throw an exception in that case to provide a proper sentinel.
    template<typename T>
    auto GetProperty(const std::string& key) const -> T { return fConfig->GetValue<T>(key); }

    /// @brief Read config property as string
    /// @param key
    /// @return config property value converted to string
    /// 
    /// If a type is not supported, the user can provide support by overloading the ostream operator for this type
    auto GetPropertyAsString(const std::string& key) const -> std::string { return fConfig->GetStringValue(key); }

    /// @brief Subscribe to property updates of type T
    /// @param subscriber
    /// @param callback function
    ///
    /// While PluginServices provides the SetProperty method which can update properties only during certain device states, there are
    /// other methods in a FairMQ device that can update properties at any time. Therefore, the callback implementation should expect to be called in any
    /// device state.
    // template<typename T>
    // auto SubscribeToPropertyChange(
    //     const std::string& subscriber,
    //     std::function<void(const std::string& [>key*/, const T /*newValue<])> callback
    // ) const -> void
    // {
    //     fConfig->Subscribe(subscriber, callback);
    // }
    //
    // /// @brief Unsubscribe from property updates of type T
    // /// @param subscriber
    // template<typename T>
    // auto UnsubscribeFromPropertyChange(const std::string& subscriber) -> void { fConfig->Unsubscribe<T>(subscriber); }
    //
    // TODO Fix property subscription
    // TODO Property iterator

    static const std::unordered_map<std::string, DeviceState> fkDeviceStateStrMap;
    static const std::unordered_map<DeviceState, std::string, tools::HashEnum<DeviceState>> fkStrDeviceStateMap;
    static const std::unordered_map<std::string, DeviceStateTransition> fkDeviceStateTransitionStrMap;
    static const std::unordered_map<DeviceStateTransition, std::string, tools::HashEnum<DeviceStateTransition>> fkStrDeviceStateTransitionMap;
    static const std::unordered_map<FairMQDevice::State, DeviceState, tools::HashEnum<FairMQDevice::State>> fkDeviceStateMap;
    static const std::unordered_map<DeviceStateTransition, FairMQDevice::Event, tools::HashEnum<DeviceStateTransition>> fkDeviceStateTransitionMap;

    private:

    FairMQProgOptions* fConfig; // TODO make it a shared pointer, once old AliceO2 code is cleaned up
    std::shared_ptr<FairMQDevice> fDevice;
}; /* class PluginServices */

} /* namespace mq */
} /* namespace fair */

#endif /* FAIR_MQ_PLUGINSERVICES_H */
