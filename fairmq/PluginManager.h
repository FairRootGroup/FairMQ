/********************************************************************************
 * Copyright (C) 2017-2023 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
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

#if FAIRMQ_HAS_STD_FILESYSTEM
#include <filesystem>
namespace fs = std::filesystem;
#else
#define BOOST_FILESYSTEM_VERSION 3
#define BOOST_FILESYSTEM_NO_DEPRECATED
#include <boost/filesystem.hpp>
namespace fs = ::boost::filesystem;
#endif

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

namespace fair::mq
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
    PluginManager(const PluginManager&) = delete;
    PluginManager(PluginManager&&) = delete;
    PluginManager& operator=(const PluginManager&) = delete;
    PluginManager& operator=(PluginManager&&) = delete;
    PluginManager(const std::vector<std::string>& args);

    ~PluginManager()
    {
        LOG(debug) << "Shutting down Plugin Manager";
    }

    auto SetSearchPaths(const std::vector<fs::path>&) -> void;
    auto AppendSearchPath(const fs::path&) -> void;
    auto PrependSearchPath(const fs::path&) -> void;
    auto SearchPaths() const -> const std::vector<fs::path>& { return fSearchPaths; }
    struct BadSearchPath : std::invalid_argument { using std::invalid_argument::invalid_argument; };

    auto SearchPluginFile(const std::string&) const -> fs::path;
    struct PluginNotFound : std::runtime_error { using std::runtime_error::runtime_error; };
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
    static auto ValidateSearchPath(const fs::path&) -> void;

    auto LoadPluginPrelinkedDynamic(const std::string& pluginName) -> void;
    auto LoadPluginDynamic(const std::string& pluginName) -> void;
    auto LoadPluginStatic(const std::string& pluginName) -> void;
#if FAIRMQ_HAS_STD_FILESYSTEM
    template<typename T>
    static auto AdaptPathType(T&& path)
    {
        if constexpr(std::is_same_v<T, std::filesystem::path>) {
            return boost::filesystem::path(std::forward<T>(path));
        } else {
            return std::forward<T>(path);
        }
    }
#endif
    template<typename FirstArg, typename... Args>
    auto LoadSymbols(const std::string& pluginName, FirstArg&& farg, Args&&... args) -> void
    {
        using namespace boost::dll;
        using fair::mq::tools::ToString;

#if FAIRMQ_HAS_STD_FILESYSTEM
        auto lib = shared_library{AdaptPathType(std::forward<FirstArg>(farg)), std::forward<Args>(args)...};
#else
        auto lib = shared_library{std::forward<FirstArg>(farg), std::forward<Args>(args)...};
#endif
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
    static const std::string fgkLibPrefixAlt;
    std::vector<fs::path> fSearchPaths;
    static std::vector<boost::dll::shared_library> fgDLLKeepAlive; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
    std::map<std::string, std::function<PluginFactory>> fPluginFactories;
    std::unique_ptr<PluginServices> fPluginServices;
    std::map<std::string, std::unique_ptr<Plugin>> fPlugins;
    std::vector<std::string> fPluginOrder;
    std::map<std::string, boost::program_options::options_description> fPluginProgOptions;
}; /* class PluginManager */

} // namespace fair::mq

#endif /* FAIR_MQ_PLUGINMANAGER_H */
