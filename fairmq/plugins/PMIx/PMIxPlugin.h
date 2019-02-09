/********************************************************************************
 *    Copyright (C) 2019 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_PLUGINS_PMIX
#define FAIR_MQ_PLUGINS_PMIX

#include "PMIx.hpp"

#include <fairmq/Plugin.h>
#include <fairmq/Version.h>
#include <FairMQLogger.h>

#include <string>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

namespace fair
{
namespace mq
{
namespace plugins
{

class PMIxPlugin : public Plugin
{
  public:
    PMIxPlugin(const std::string& name,
         const Plugin::Version version,
         const std::string& maintainer,
         const std::string& homepage,
         PluginServices* pluginServices);
    ~PMIxPlugin();
    auto PMIxClient() const -> std::string;

  private:
    pmix::proc fProc;
    pid_t fPid;

    auto Init() -> void;
    auto Publish() -> void;
    auto Fence() -> void;
    auto Lookup() -> void;
};

Plugin::ProgOptions PMIxProgramOptions()
{
    boost::program_options::options_description options{"PMIx Plugin"};
    options.add_options()(
        "pmix-dummy", boost::program_options::value<int>()->default_value(0), "Dummy.");
    return options;
}

REGISTER_FAIRMQ_PLUGIN(
    PMIxPlugin,                                  // Class name
    pmix,                                        // Plugin name (string, lower case chars only)
    (Plugin::Version{FAIRMQ_VERSION_MAJOR,
                     FAIRMQ_VERSION_MINOR,
                     FAIRMQ_VERSION_PATCH}),     // Version
    "FairRootGroup <fairroot@gsi.de>",           // Maintainer
    "https://github.com/FairRootGroup/FairMQ",   // Homepage
    PMIxProgramOptions                           // custom program options for the plugin
)

} /* namespace plugins */
} /* namespace mq */
} /* namespace fair */

#endif /* FAIR_MQ_PLUGINS_PMIX */
