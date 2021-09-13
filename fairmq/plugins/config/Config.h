/********************************************************************************
 *    Copyright (C) 2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_PLUGINS_CONFIG
#define FAIR_MQ_PLUGINS_CONFIG

#include <fairmq/Plugin.h>
#include <fairmq/Version.h>

#include <string>

namespace fair::mq::plugins
{

class Config : public Plugin
{
  public:
    Config(const std::string& name, Plugin::Version version, const std::string& maintainer, const std::string& homepage, PluginServices* pluginServices);

    ~Config();
};

Plugin::ProgOptions ConfigPluginProgramOptions();

REGISTER_FAIRMQ_PLUGIN(
    Config,   // Class name
    config,   // Plugin name
    (Plugin::Version{FAIRMQ_VERSION_MAJOR, FAIRMQ_VERSION_MINOR, FAIRMQ_VERSION_PATCH}),
    "FairRootGroup <fairroot@gsi.de>",
    "https://github.com/FairRootGroup/FairRoot",
    ConfigPluginProgramOptions
)

} // namespace fair::mq::plugins

#endif /* FAIR_MQ_PLUGINS_CONFIG */
