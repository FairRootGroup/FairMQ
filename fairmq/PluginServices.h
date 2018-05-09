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

#include <boost/optional.hpp>
#include <boost/optional/optional_io.hpp>

#include <functional>
#include <string>
#include <unordered_map>
#include <mutex>
#include <condition_variable>

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
        : fConfig{config}
        , fDevice{device}
        , fDeviceController()
        , fDeviceControllerMutex()
        , fReleaseDeviceControlCondition()
    {
    }

    PluginServices(const PluginServices&) = delete;
    PluginServices operator=(const PluginServices&) = delete;

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
        Resume,
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

    /// @brief Become device controller
    /// @param controller id
    /// @throws fair::mq::PluginServices::DeviceControlError if there is already a device controller.
    ///
    /// Only one plugin can succeed to take control over device state transitions at a time.
    auto TakeDeviceControl(const std::string& controller) -> void;
    struct DeviceControlError : std::runtime_error { using std::runtime_error::runtime_error; };

    /// @brief Become device controller by force
    /// @param controller id
    ///
    /// Take over device controller privileges by force. Does not trigger the ReleaseDeviceControl condition!
    /// This function is intended to implement override/emergency control functionality (e.g. device shutdown on SIGINT).
    auto StealDeviceControl(const std::string& controller) -> void;

    /// @brief Release device controller role
    /// @param controller id
    /// @throws fair::mq::PluginServices::DeviceControlError if passed controller id is not the current device controller.
    auto ReleaseDeviceControl(const std::string& controller) -> void;

    /// @brief Get current device controller
    auto GetDeviceController() const -> boost::optional<std::string>;

    /// @brief Block until control is released
    auto WaitForReleaseDeviceControl() -> void;

    /// @brief Request a device state transition
    /// @param controller id
    /// @param next state transition
    /// @throws fair::mq::PluginServices::DeviceControlError if control role is not currently owned by passed controller id.
    ///
    /// The state transition may not happen immediately, but when the current state evaluates the
    /// pending transition event and terminates. In other words, the device states are scheduled cooperatively.
    /// If the device control role has not been taken yet, calling this function will take over control implicitely.
    auto ChangeDeviceState(const std::string& controller, const DeviceStateTransition next) -> void;

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

    /// @brief Unsubscribe from device state changes
    /// @param subscriber id
    auto UnsubscribeFromDeviceStateChange(const std::string& subscriber) -> void { fDevice->UnsubscribeFromStateChange(subscriber); }

    // Config API
    struct PropertyNotFoundError : std::runtime_error { using std::runtime_error::runtime_error; };

    auto PropertyExists(const std::string& key) const -> bool { return fConfig->Count(key) > 0; }

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
    auto GetProperty(const std::string& key) const -> T {
        if (PropertyExists(key)) {
            return fConfig->GetValue<T>(key);
        }
        throw PropertyNotFoundError(fair::mq::tools::ToString("Config has no key: ", key));
    }

    /// @brief Read config property as string
    /// @param key
    /// @return config property value converted to string
    ///
    /// If a type is not supported, the user can provide support by overloading the ostream operator for this type
    auto GetPropertyAsString(const std::string& key) const -> std::string {
        if (PropertyExists(key)) {
            return fConfig->GetStringValue(key);
        }
        throw PropertyNotFoundError(fair::mq::tools::ToString("Config has no key: ", key));
    }

    auto GetChannelInfo() const -> std::unordered_map<std::string, int> { return fConfig->GetChannelInfo(); }

    /// @brief Discover the list of property keys
    /// @return list of property keys
    auto GetPropertyKeys() const -> std::vector<std::string> { return fConfig->GetPropertyKeys(); }

    /// @brief Subscribe to property updates of type T
    /// @param subscriber
    /// @param callback function
    ///
    /// Subscribe to property changes with a callback to monitor property changes in an event based fashion.
    template<typename T>
    auto SubscribeToPropertyChange(const std::string& subscriber, std::function<void(const std::string& key, T)> callback) const -> void
    {
        fConfig->Subscribe<T>(subscriber, callback);
    }

    /// @brief Unsubscribe from property updates of type T
    /// @param subscriber
    template<typename T>
    auto UnsubscribeFromPropertyChange(const std::string& subscriber) -> void { fConfig->Unsubscribe<T>(subscriber); }

    /// @brief Subscribe to property updates
    /// @param subscriber
    /// @param callback function
    ///
    /// Subscribe to property changes with a callback to monitor property changes in an event based fashion. Will convert the property to string.
    auto SubscribeToPropertyChangeAsString(const std::string& subscriber, std::function<void(const std::string& key, std::string)> callback) const -> void
    {
        fConfig->SubscribeAsString(subscriber, callback);
    }

    /// @brief Unsubscribe from property updates that convert to string
    /// @param subscriber
    auto UnsubscribeFromPropertyChangeAsString(const std::string& subscriber) -> void { fConfig->UnsubscribeAsString(subscriber); }

    auto CycleLogConsoleSeverityUp() -> void { Logger::CycleConsoleSeverityUp(); }
    auto CycleLogConsoleSeverityDown() -> void { Logger::CycleConsoleSeverityDown(); }
    auto CycleLogVerbosityUp() -> void { Logger::CycleVerbosityUp(); }
    auto CycleLogVerbosityDown() -> void { Logger::CycleVerbosityDown(); }

    static const std::unordered_map<std::string, DeviceState> fkDeviceStateStrMap;
    static const std::unordered_map<DeviceState, std::string, tools::HashEnum<DeviceState>> fkStrDeviceStateMap;
    static const std::unordered_map<std::string, DeviceStateTransition> fkDeviceStateTransitionStrMap;
    static const std::unordered_map<DeviceStateTransition, std::string, tools::HashEnum<DeviceStateTransition>> fkStrDeviceStateTransitionMap;
    static const std::unordered_map<FairMQDevice::State, DeviceState, tools::HashEnum<FairMQDevice::State>> fkDeviceStateMap;
    static const std::unordered_map<DeviceStateTransition, FairMQDevice::Event, tools::HashEnum<DeviceStateTransition>> fkDeviceStateTransitionMap;

  private:
    FairMQProgOptions* fConfig; // TODO make it a shared pointer, once old AliceO2 code is cleaned up
    std::shared_ptr<FairMQDevice> fDevice;
    boost::optional<std::string> fDeviceController;
    mutable std::mutex fDeviceControllerMutex;
    std::condition_variable fReleaseDeviceControlCondition;
}; /* class PluginServices */

} /* namespace mq */
} /* namespace fair */

#endif /* FAIR_MQ_PLUGINSERVICES_H */
