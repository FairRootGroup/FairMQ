/********************************************************************************
 * Copyright (C) 2017-2019 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_PLUGIN_H
#define FAIR_MQ_PLUGIN_H

#include <fairmq/tools/Version.h>
#include <fairmq/PluginServices.h>

#include <boost/dll/alias.hpp>
#include <boost/optional.hpp>
#include <boost/program_options.hpp>

#include <functional>
#include <unordered_map>
#include <ostream>
#include <memory>
#include <string>
#include <tuple>
#include <utility>

namespace fair::mq
{

/**
 * @class Plugin Plugin.h <fairmq/Plugin.h>
 * @brief Base class for FairMQ plugins
 *
 * The plugin base class encapsulates the plugin metadata.
 */
class Plugin
{
  public:
    using ProgOptions = boost::optional<boost::program_options::options_description>;

    using Version = tools::Version;

    Plugin() = delete;
    Plugin(std::string name,
           Version version,
           std::string maintainer,
           std::string homepage,
           PluginServices* pluginServices);

    Plugin(const Plugin&) = delete;
    Plugin operator=(const Plugin&) = delete;

    virtual ~Plugin();

    auto GetName() const -> const std::string& { return fkName; }
    auto GetVersion() const -> const Version { return fkVersion; }
    auto GetMaintainer() const -> const std::string& { return fkMaintainer; }
    auto GetHomepage() const -> const std::string& { return fkHomepage; }

    friend auto operator==(const Plugin& lhs, const Plugin& rhs) -> bool { return std::make_tuple(lhs.GetName(), lhs.GetVersion()) == std::make_tuple(rhs.GetName(), rhs.GetVersion()); }
    friend auto operator!=(const Plugin& lhs, const Plugin& rhs) -> bool { return !(lhs == rhs); }
    friend auto operator<<(std::ostream& os, const Plugin& p) -> std::ostream&
    {
        return os << "'"            << p.GetName()       << "', "
                  << "version '"    << p.GetVersion()    << "', "
                  << "maintainer '" << p.GetMaintainer() << "', "
                  << "homepage '"   << p.GetHomepage()   << "'";
    }
    static auto NoProgramOptions() -> ProgOptions { return boost::none; }

    // device control API
    // see <fairmq/PluginServices.h> for docs
    using DeviceState = fair::mq::PluginServices::DeviceState;
    using DeviceStateTransition = fair::mq::PluginServices::DeviceStateTransition;
    auto ToDeviceState(const std::string& state) const -> DeviceState { return fPluginServices->ToDeviceState(state); }
    auto ToDeviceStateTransition(const std::string& transition) const -> DeviceStateTransition { return fPluginServices->ToDeviceStateTransition(transition); }
    auto ToStr(DeviceState state) const -> std::string { return fPluginServices->ToStr(state); }
    auto ToStr(DeviceStateTransition transition) const -> std::string { return fPluginServices->ToStr(transition); }
    auto GetCurrentDeviceState() const -> DeviceState { return fPluginServices->GetCurrentDeviceState(); }
    auto TakeDeviceControl() -> void { fPluginServices->TakeDeviceControl(fkName); };
    auto StealDeviceControl() -> void { fPluginServices->StealDeviceControl(fkName); };
    auto ReleaseDeviceControl() -> void { fPluginServices->ReleaseDeviceControl(fkName); };
    auto ChangeDeviceState(const DeviceStateTransition next) -> bool { return fPluginServices->ChangeDeviceState(fkName, next); }
    auto SubscribeToDeviceStateChange(std::function<void(DeviceState)> callback) -> void { fPluginServices->SubscribeToDeviceStateChange(fkName, callback); }
    auto UnsubscribeFromDeviceStateChange() -> void { fPluginServices->UnsubscribeFromDeviceStateChange(fkName); }

    // device config API
    // see <fairmq/PluginServices.h> for docs
    auto PropertyExists(const std::string& key) -> int { return fPluginServices->PropertyExists(key); }

    template<typename T>
    T GetProperty(const std::string& key) const { return fPluginServices->GetProperty<T>(key); }
    template<typename T>
    T GetProperty(const std::string& key, const T& ifNotFound) const { return fPluginServices->GetProperty(key, ifNotFound); }
    std::string GetPropertyAsString(const std::string& key) const { return fPluginServices->GetPropertyAsString(key); }
    std::string GetPropertyAsString(const std::string& key, const std::string& ifNotFound) const  { return fPluginServices->GetPropertyAsString(key, ifNotFound); }
    fair::mq::Properties GetProperties(const std::string& q) const { return fPluginServices->GetProperties(q); }
    fair::mq::Properties GetPropertiesStartingWith(const std::string& q) const { return fPluginServices->GetPropertiesStartingWith(q); };
    std::map<std::string, std::string> GetPropertiesAsString(const std::string& q) const { return fPluginServices->GetPropertiesAsString(q); }
    std::map<std::string, std::string> GetPropertiesAsStringStartingWith(const std::string& q) const { return fPluginServices->GetPropertiesAsStringStartingWith(q); };

    auto GetChannelInfo() const -> std::unordered_map<std::string, int> { return fPluginServices->GetChannelInfo(); }
    auto GetPropertyKeys() const -> std::vector<std::string> { return fPluginServices->GetPropertyKeys(); }

    template<typename T>
    auto SetProperty(const std::string& key, T val) -> void { fPluginServices->SetProperty(key, val); }
    void SetProperties(const fair::mq::Properties& props) { fPluginServices->SetProperties(props); }
    template<typename T>
    bool UpdateProperty(const std::string& key, T val) { return fPluginServices->UpdateProperty(key, val); }
    bool UpdateProperties(const fair::mq::Properties& input) { return fPluginServices->UpdateProperties(input); }

    void DeleteProperty(const std::string& key) { fPluginServices->DeleteProperty(key); }

    template<typename T>
    auto SubscribeToPropertyChange(std::function<void(const std::string& key, T newValue)> callback) -> void { fPluginServices->SubscribeToPropertyChange<T>(fkName, callback); }
    template<typename T>
    auto UnsubscribeFromPropertyChange() -> void { fPluginServices->UnsubscribeFromPropertyChange<T>(fkName); }
    auto SubscribeToPropertyChangeAsString(std::function<void(const std::string& key, std::string newValue)> callback) -> void { fPluginServices->SubscribeToPropertyChangeAsString(fkName, callback); }
    auto UnsubscribeFromPropertyChangeAsString() -> void { fPluginServices->UnsubscribeFromPropertyChangeAsString(fkName); }

    auto CycleLogConsoleSeverityUp() -> void { fPluginServices->CycleLogConsoleSeverityUp(); }
    auto CycleLogConsoleSeverityDown() -> void { fPluginServices->CycleLogConsoleSeverityDown(); }
    auto CycleLogVerbosityUp() -> void { fPluginServices->CycleLogVerbosityUp(); }
    auto CycleLogVerbosityDown() -> void { fPluginServices->CycleLogVerbosityDown(); }

  private:
    const std::string fkName;
    const Version fkVersion;
    const std::string fkMaintainer;
    const std::string fkHomepage;
    PluginServices* fPluginServices;
}; /* class Plugin */

} // namespace fair::mq

#define REGISTER_FAIRMQ_PLUGIN(KLASS, NAME, VERSION, MAINTAINER, HOMEPAGE, PROGOPTIONS) \
static auto Make_##NAME##_Plugin(fair::mq::PluginServices* pluginServices) -> std::unique_ptr<fair::mq::Plugin> \
{ \
    return std::make_unique<KLASS>(std::string{#NAME}, VERSION, std::string{MAINTAINER}, std::string{HOMEPAGE}, pluginServices); \
} \
BOOST_DLL_ALIAS(Make_##NAME##_Plugin, make_##NAME##_plugin) \
BOOST_DLL_ALIAS(PROGOPTIONS, get_##NAME##_plugin_progoptions)

#endif /* FAIR_MQ_PLUGIN_H */
