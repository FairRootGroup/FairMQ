/********************************************************************************
 *    Copyright (C) 2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_PLUGIN_H
#define FAIR_MQ_PLUGIN_H

#include <fairmq/Tools.h>
#include <fairmq/PluginServices.h>
#include <boost/dll/alias.hpp>
#include <boost/optional.hpp>
#include <boost/program_options.hpp>
#include <functional>
#include <ostream>
#include <memory>
#include <string>
#include <tuple>
#include <utility>

namespace fair
{
namespace mq
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
    
    struct Version
    {
        const int fkMajor, fkMinor, fkPatch;

        friend auto operator< (const Version& lhs, const Version& rhs) -> bool { return std::tie(lhs.fkMajor, lhs.fkMinor, lhs.fkPatch) < std::tie(rhs.fkMajor, rhs.fkMinor, rhs.fkPatch); }
        friend auto operator> (const Version& lhs, const Version& rhs) -> bool { return rhs < lhs; }
        friend auto operator<=(const Version& lhs, const Version& rhs) -> bool { return !(lhs > rhs); }
        friend auto operator>=(const Version& lhs, const Version& rhs) -> bool { return !(lhs < rhs); }
        friend auto operator==(const Version& lhs, const Version& rhs) -> bool { return std::tie(lhs.fkMajor, lhs.fkMinor, lhs.fkPatch) == std::tie(rhs.fkMajor, rhs.fkMinor, rhs.fkPatch); }
        friend auto operator!=(const Version& lhs, const Version& rhs) -> bool { return !(lhs == rhs); }
        friend auto operator<<(std::ostream& os, const Version& v) -> std::ostream& { return os << v.fkMajor << "." << v.fkMinor << "." << v.fkPatch; }
    };

    Plugin() = delete;
    Plugin(const std::string name, const Version version, const std::string maintainer, const std::string homepage, PluginServices& pluginServices);
    virtual ~Plugin();

    auto GetName() const -> const std::string& { return fkName; }
    auto GetVersion() const -> const Version { return fkVersion; }
    auto GetMaintainer() const -> const std::string& { return fkMaintainer; }
    auto GetHomepage() const -> const std::string& { return fkHomepage; }

    friend auto operator==(const Plugin& lhs, const Plugin& rhs) -> bool { return std::make_tuple(lhs.GetName(), lhs.GetVersion()) == std::make_tuple(rhs.GetName(), rhs.GetVersion()); }
    friend auto operator!=(const Plugin& lhs, const Plugin& rhs) -> bool { return !(lhs == rhs); }
    friend auto operator<<(std::ostream& os, const Plugin& p) -> std::ostream&
    {
        return os << "'" << p.GetName() << "', "
                  << "version '" << p.GetVersion() << "', "
                  << "maintainer '" << p.GetMaintainer() << "', "
                  << "homepage '" << p.GetHomepage() << "'";
    }
    static auto NoProgramOptions() -> const boost::optional<boost::program_options::options_description> { return boost::none; }

    private:

    const std::string fkName;
    const Version fkVersion;
    const std::string fkMaintainer;
    const std::string fkHomepage;
    PluginServices& fPluginServices;

}; /* class Plugin */

} /* namespace mq */
} /* namespace fair */

#define REGISTER_FAIRMQ_PLUGIN(KLASS, NAME, VERSION, MAINTAINER, HOMEPAGE, PROGOPTIONS) \
static auto Make_##NAME##_Plugin(fair::mq::PluginServices& pluginServices) -> std::shared_ptr<KLASS> \
{ \
    return std::make_shared<KLASS>(std::string{#NAME}, VERSION, std::string{MAINTAINER}, std::string{HOMEPAGE}, pluginServices); \
} \
BOOST_DLL_ALIAS(Make_##NAME##_Plugin, make_##NAME##_plugin) \
BOOST_DLL_ALIAS(PROGOPTIONS, get_##NAME##_plugin_progoptions)

#endif /* FAIR_MQ_PLUGIN_H */
