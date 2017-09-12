/********************************************************************************
 *    Copyright (C) 2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_PLUGINS_CONTROLSTATIC
#define FAIR_MQ_PLUGINS_CONTROLSTATIC

#include <fairmq/Plugin.h>
#include <string>

namespace fair
{
namespace mq
{
namespace plugins
{

class ControlStatic : public Plugin
{
    public:

    ControlStatic(
        const std::string name,
        const Plugin::Version version,
        const std::string maintainer,
        const std::string homepage,
        PluginServices* pluginServices
    );
}; /* class ControlStatic */

auto ControlStaticPluginProgramOptions() -> Plugin::ProgOptions;

REGISTER_FAIRMQ_PLUGIN(
    ControlStatic,                               // Class name
    control_static,                              // Plugin name (string, lower case chars only)
    (Plugin::Version{1,0,0}),                    // Version
    "FairRootGroup <fairroot@gsi.de>",           // Maintainer
    "https://github.com/FairRootGroup/FairRoot", // Homepage
    ControlStaticPluginProgramOptions            // Free function which declares custom program options for the plugin
                                                 // signature: () -> boost::optional<boost::program_options::options_description>
)

} /* namespace plugins */
} /* namespace mq */
} /* namespace fair */

#endif /* FAIR_MQ_PLUGINS_CONTROLSTATIC */
