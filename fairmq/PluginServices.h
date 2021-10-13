/********************************************************************************
 * Copyright (C) 2017-2021 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_PLUGINSERVICES_H
#define FAIR_MQ_PLUGINSERVICES_H

#include <fairmq/Device.h>
#include <fairmq/States.h>
#include <fairmq/ProgOptions.h>
#include <fairmq/Properties.h>

#include <boost/optional.hpp>
#include <boost/optional/optional_io.hpp>

#include <condition_variable>
#include <functional>
#include <map>
#include <mutex>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace fair::mq
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
    PluginServices(ProgOptions& config, Device& device)
        : fConfig(config)
        , fDevice(device)
    {}

    ~PluginServices()
    {
        LOG(debug) << "Shutting down Plugin Services";
    }

    PluginServices(const PluginServices&) = delete;
    PluginServices(PluginServices&&) = delete;
    PluginServices& operator=(const PluginServices&) = delete;
    PluginServices& operator=(PluginServices&&) = delete;

    using DeviceState = fair::mq::State;
    using DeviceStateTransition = fair::mq::Transition;

    // Control API

    /// @brief Convert string to DeviceState
    /// @param state to convert
    /// @return DeviceState enum entry
    /// @throw std::out_of_range if a string cannot be resolved to a DeviceState
    static auto ToDeviceState(const std::string& state) -> DeviceState { return GetState(state); }

    /// @brief Convert string to DeviceStateTransition
    /// @param transition to convert
    /// @return DeviceStateTransition enum entry
    /// @throw std::out_of_range if a string cannot be resolved to a DeviceStateTransition
    static auto ToDeviceStateTransition(const std::string& transition) -> DeviceStateTransition { return GetTransition(transition); }

    /// @brief Convert DeviceState to string
    /// @param state to convert
    /// @return string representation of DeviceState enum entry
    static auto ToStr(DeviceState state) -> std::string { return GetStateName(state); }

    /// @brief Convert DeviceStateTransition to string
    /// @param transition to convert
    /// @return string representation of DeviceStateTransition enum entry
    static auto ToStr(DeviceStateTransition transition) -> std::string { return GetTransitionName(transition); }

    /// @return current device state
    auto GetCurrentDeviceState() const -> DeviceState { return fDevice.GetCurrentState(); }

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
    auto ChangeDeviceState(const std::string& controller, DeviceStateTransition next) -> bool;

    /// @brief Subscribe with a callback to device state changes
    /// @param subscriber id
    /// @param callback
    ///
    /// The callback will be called at the beginning of a new state. The callback is called from the thread
    /// the state is running in.
    auto SubscribeToDeviceStateChange(const std::string& subscriber, std::function<void(DeviceState /*newState*/)> callback) -> void
    {
        fDevice.SubscribeToStateChange(subscriber, [&,callback](fair::mq::State newState){
            callback(newState);
        });
    }

    /// @brief Unsubscribe from device state changes
    /// @param subscriber id
    auto UnsubscribeFromDeviceStateChange(const std::string& subscriber) -> void { fDevice.UnsubscribeFromStateChange(subscriber); }

    /// DO NOT USE, ONLY FOR TESTING, WILL BE REMOVED (and info made available via property api)
    auto GetNumberOfConnectedPeers(const std::string& channelName, int index = 0) -> unsigned long { return fDevice.GetNumberOfConnectedPeers(channelName, index); }

    // Config API

    /// @brief Checks a property with the given key exist in the configuration
    /// @param key
    /// @return true if it exists, false otherwise
    auto PropertyExists(const std::string& key) const -> bool { return fConfig.Count(key) > 0; }

    /// @brief Set config property
    /// @param key
    /// @param val
    ///
    /// Setting a config property will store the value in the FairMQ internal config store and notify any subscribers about the update.
    /// It is property dependent, if the call to this method will have an immediate, delayed or any effect at all.
    template<typename T>
    auto SetProperty(const std::string& key, T val) -> void { fConfig.SetProperty(key, val); }
    /// @brief Set multiple config properties
    /// @param props fair::mq::Properties as an alias for std::map<std::string, Property>, where property is boost::any
    void SetProperties(const fair::mq::Properties& props) { fConfig.SetProperties(props); }
    /// @brief Updates an existing config property (or fails if it doesn't exist)
    /// @param key
    /// @param val
    template<typename T>
    bool UpdateProperty(const std::string& key, T val) { return fConfig.UpdateProperty(key, val); }
    /// @brief Updates multiple existing config properties (or fails of any of then do not exist, leaving property store unchanged)
    /// @param props (fair::mq::Properties as an alias for std::map<std::string, Property>, where property is boost::any)
    bool UpdateProperties(const fair::mq::Properties& input) { return fConfig.UpdateProperties(input); }

    /// @brief Deletes a property with the given key from the config store
    /// @param key
    void DeleteProperty(const std::string& key) { fConfig.DeleteProperty(key); }

    /// @brief Read config property, throw if no property with this key exists
    /// @param key
    /// @return config property
    template<typename T>
    auto GetProperty(const std::string& key) const -> T { return fConfig.GetProperty<T>(key); }

    /// @brief Read config property, return provided value if no property with this key exists
    /// @param key
    /// @param ifNotFound value to return if key is not found
    /// @return config property
    template<typename T>
    T GetProperty(const std::string& key, const T& ifNotFound) const { return fConfig.GetProperty(key, ifNotFound); }

    /// @brief Read config property as string, throw if no property with this key exists
    /// @param key
    /// @return config property converted to string
    ///
    /// Supports conversion to string for a fixed set of types,
    /// for custom/unsupported types add them via `fair::mq::PropertyHelper::AddType<MyType>("optional label")`
    /// the provided type must then be convertible to string via operator<<
    auto GetPropertyAsString(const std::string& key) const -> std::string { return fConfig.GetPropertyAsString(key); }

    /// @brief Read config property, return provided value if no property with this key exists
    /// @param key
    /// @param ifNotFound value to return if key is not found
    /// @return config property converted to string
    ///
    /// Supports conversion to string for a fixed set of types,
    /// for custom/unsupported types add them via `fair::mq::PropertyHelper::AddType<MyType>("optional label")`
    /// the provided type must then be convertible to string via operator<<
    auto GetPropertyAsString(const std::string& key, const std::string& ifNotFound) const -> std::string { return fConfig.GetPropertyAsString(key, ifNotFound); }

    /// @brief Read several config properties whose keys match the provided regular expression
    /// @param q regex string to match for
    /// @return container with properties (fair::mq::Properties as an alias for std::map<std::string, Property>, where property is boost::any)
    fair::mq::Properties GetProperties(const std::string& q) const { return fConfig.GetProperties(q); }
    /// @brief Read several config properties whose keys start with the provided string
    /// @param q string to match for
    /// @return container with properties (fair::mq::Properties as an alias for std::map<std::string, Property>, where property is boost::any)
    ///
    /// Typically more performant than GetProperties with regex
    fair::mq::Properties GetPropertiesStartingWith(const std::string& q) const { return fConfig.GetPropertiesStartingWith(q); }
    /// @brief Read several config properties as string whose keys match the provided regular expression
    /// @param q regex string to match for
    /// @return container with properties (fair::mq::Properties as an alias for std::map<std::string, Property>, where property is boost::any)
    std::map<std::string, std::string> GetPropertiesAsString(const std::string& q) const { return fConfig.GetPropertiesAsString(q); }
    /// @brief Read several config properties as string whose keys start with the provided string
    /// @param q string to match for
    /// @return container with properties (fair::mq::Properties as an alias for std::map<std::string, Property>, where property is boost::any)
    ///
    /// Typically more performant than GetPropertiesAsString with regex
    std::map<std::string, std::string> GetPropertiesAsStringStartingWith(const std::string& q) const { return fConfig.GetPropertiesAsStringStartingWith(q); }

    /// @brief Retrieve current channel information
    /// @return a map of <channel name, number of subchannels>
    auto GetChannelInfo() const -> std::unordered_map<std::string, int> { return fConfig.GetChannelInfo(); }

    /// @brief Discover the list of property keys
    /// @return list of property keys
    auto GetPropertyKeys() const -> std::vector<std::string> { return fConfig.GetPropertyKeys(); }

    /// @brief Subscribe to property updates of type T
    /// @param subscriber
    /// @param callback function
    ///
    /// Subscribe to property changes with a callback to monitor property changes in an event based fashion.
    template<typename T>
    auto SubscribeToPropertyChange(const std::string& subscriber, std::function<void(const std::string& key, T)> callback) const -> void
    {
        fConfig.Subscribe<T>(subscriber, callback);
    }

    /// @brief Unsubscribe from property updates of type T
    /// @param subscriber
    template<typename T>
    auto UnsubscribeFromPropertyChange(const std::string& subscriber) -> void { fConfig.Unsubscribe<T>(subscriber); }

    /// @brief Subscribe to property updates
    /// @param subscriber
    /// @param callback function
    ///
    /// Subscribe to property changes with a callback to monitor property changes in an event based fashion. Will convert the property to string.
    auto SubscribeToPropertyChangeAsString(const std::string& subscriber, std::function<void(const std::string& key, std::string)> callback) const -> void
    {
        fConfig.SubscribeAsString(subscriber, callback);
    }

    /// @brief Unsubscribe from property updates that convert to string
    /// @param subscriber
    auto UnsubscribeFromPropertyChangeAsString(const std::string& subscriber) -> void { fConfig.UnsubscribeAsString(subscriber); }

    /// @brief Increases console logging severity, or sets it to lowest if it is already highest
    auto CycleLogConsoleSeverityUp() -> void { Logger::CycleConsoleSeverityUp(); }
    /// @brief Decreases console logging severity, or sets it to highest if it is already lowest
    auto CycleLogConsoleSeverityDown() -> void { Logger::CycleConsoleSeverityDown(); }
    /// @brief Increases logging verbosity, or sets it to lowest if it is already highest
    auto CycleLogVerbosityUp() -> void { Logger::CycleVerbosityUp(); }
    /// @brief Decreases logging verbosity, or sets it to highest if it is already lowest
    auto CycleLogVerbosityDown() -> void { Logger::CycleVerbosityDown(); }

  private:
    fair::mq::ProgOptions& fConfig;
    Device& fDevice;
    boost::optional<std::string> fDeviceController;
    mutable std::mutex fDeviceControllerMutex;
    std::condition_variable fReleaseDeviceControlCondition;
}; /* class PluginServices */

} // namespace fair::mq

#endif /* FAIR_MQ_PLUGINSERVICES_H */
