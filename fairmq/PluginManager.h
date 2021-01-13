/********************************************************************************
 *    Copyright (C) 2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_PLUGINMANAGER_H
#define FAIR_MQ_PLUGINMANAGER_H

#include <fairmq/Plugin.h>
#include <fairmq/PluginServices.h>
#include <fairmq/tools/Strings.h>

#define BOOST_FILESYSTEM_VERSION 3
#define BOOST_FILESYSTEM_NO_DEPRECATED
#include <boost/filesystem.hpp>
#include <boost/optional.hpp>
#include <boost/program_options.hpp>
#include <boost/dll/import.hpp>
#include <boost/dll/shared_library.hpp>
#include <boost/dll/runtime_symbol_info.hpp>

#include <functional>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
#include <utility> // forward

namespace fair
{
namespace mq
{

/**
 * @class PluginManager PluginManager.h <fairmq/PluginManager.h>
 * @brief manages and owns plugin instances
 *
 * The plugin manager is responsible for the whole plugin lifecycle. It
 * facilitates two plugin mechanisms:
 * A prelinked dynamic plugins (shared libraries)
 * B dynamic plugins (shared libraries)
 * C static plugins (builtin)
 */
class PluginManager
{
  public:
    using PluginFactory = std::unique_ptr<fair::mq::Plugin>(PluginServices&);

    PluginManager();
    PluginManager(const std::vector<std::string> args);

    ~PluginManager()
    {
        LOG(debug) << "Shutting down Plugin Manager";
    }

    auto SetSearchPaths(const std::vector<boost::filesystem::path>&) -> void;
    auto AppendSearchPath(const boost::filesystem::path&) -> void;
    auto PrependSearchPath(const boost::filesystem::path&) -> void;
    auto SearchPaths() const -> const std::vector<boost::filesystem::path>& { return fSearchPaths; }
    struct BadSearchPath : std::invalid_argument { using std::invalid_argument::invalid_argument; };

    auto LoadPlugin(const std::string& pluginName) -> void;
    auto LoadPlugins(const std::vector<std::string>& pluginNames) -> void { for(const auto& pluginName : pluginNames) { LoadPlugin(pluginName); } }
    struct PluginLoadError : std::runtime_error { using std::runtime_error::runtime_error; };
    auto InstantiatePlugins() -> void;
    struct PluginInstantiationError : std::runtime_error { using std::runtime_error::runtime_error; };

    static auto ProgramOptions() -> boost::program_options::options_description;
    struct ProgramOptionsParseError : std::runtime_error { using std::runtime_error::runtime_error; };

    static auto LibPrefix() -> const std::string& { return fgkLibPrefix; }

    auto ForEachPlugin(std::function<void (Plugin&)> func) -> void { for(const auto& p : fPluginOrder) { func(*fPlugins[p]); } }
    auto ForEachPluginProgOptions(std::function<void (boost::program_options::options_description)> func) const -> void { for(const auto& pair : fPluginProgOptions) { func(pair.second); } }

    template<typename... Args>
    auto EmplacePluginServices(Args&&... args) -> void { fPluginServices = std::make_unique<PluginServices>(std::forward<Args>(args)...); }

    auto WaitForPluginsToReleaseDeviceControl() -> void { fPluginServices->WaitForReleaseDeviceControl(); }

  private:
    static auto ValidateSearchPath(const boost::filesystem::path&) -> void;

    auto LoadPluginPrelinkedDynamic(const std::string& pluginName) -> void;
    auto LoadPluginDynamic(const std::string& pluginName) -> void;
    auto LoadPluginStatic(const std::string& pluginName) -> void;
    template<typename... Args>
    auto LoadSymbols(const std::string& pluginName, Args&&... args) -> void
    {
        using namespace boost::dll;
        using fair::mq::tools::ToString;

        auto lib = shared_library{std::forward<Args>(args)...};
        fgDLLKeepAlive.push_back(lib);

        fPluginFactories[pluginName] = import_alias<PluginFactory>(
            shared_library{lib},
            ToString("make_", pluginName, "_plugin")
        );

        try
        {
            fPluginProgOptions.insert({
                pluginName,
                lib.get_alias<Plugin::ProgOptions()>(ToString("get_", pluginName, "_plugin_progoptions"))().value()
            });
        }
        catch (const boost::bad_optional_access& e) { /* just ignore, if no prog options are declared */ }
    }

    auto InstantiatePlugin(const std::string& pluginName) -> void;

    static const std::string fgkLibPrefix;
    std::vector<boost::filesystem::path> fSearchPaths;
    static std::vector<boost::dll::shared_library> fgDLLKeepAlive;
    std::map<std::string, std::function<PluginFactory>> fPluginFactories;
    std::unique_ptr<PluginServices> fPluginServices;
    std::map<std::string, std::unique_ptr<Plugin>> fPlugins;
    std::vector<std::string> fPluginOrder;
    std::map<std::string, boost::program_options::options_description> fPluginProgOptions;
}; /* class PluginManager */

} /* namespace mq */
} /* namespace fair */

#endif /* FAIR_MQ_PLUGINMANAGER_H */
