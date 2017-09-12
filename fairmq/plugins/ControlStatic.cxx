/********************************************************************************
 *    Copyright (C) 2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "ControlStatic.h"

namespace fair
{
namespace mq
{
namespace plugins
{

ControlStatic::ControlStatic(
    const std::string name,
    const Plugin::Version version,
    const std::string maintainer,
    const std::string homepage,
    PluginServices* pluginServices)
: Plugin(name, version, maintainer, homepage, pluginServices)
{
    SubscribeToDeviceStateChange(
        [&](DeviceState newState){
            LOG(WARN) << newState;
            switch (newState)
            {
                case DeviceState::InitializingDevice:
                    LOG(WARN) << GetPropertyAsString("custom-example-option");
                    SetProperty("custom-example-option", std::string{"new value"});
                break;
                case DeviceState::Exiting:
                    LOG(WARN) << GetProperty<std::string>("custom-example-option");
                    UnsubscribeFromDeviceStateChange();
                break;
            }
        }
    );
}

auto ControlStaticPluginProgramOptions() -> Plugin::ProgOptions
{
    auto plugin_options = boost::program_options::options_description{"Control Static Plugin"};
    plugin_options.add_options()
        ("custom-example-option", boost::program_options::value<std::string>(), "Custom option.");
    return plugin_options;
}

} /* namespace plugins */
} /* namespace mq */
} /* namespace fair */
