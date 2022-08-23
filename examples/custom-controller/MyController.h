/********************************************************************************
 *    Copyright (C) 2022 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_EXAMPLE_CUSTOM_CONTROLLER_PLUGIN
#define FAIR_MQ_EXAMPLE_CUSTOM_CONTROLLER_PLUGIN

#include <fairmq/Plugin.h>
#include <utility>   // for std::forward

namespace example {

struct MyController : fair::mq::Plugin   // NOLINT
{
    template<typename... Args>
    MyController(Args&&... args)
        : Plugin(std::forward<Args>(args)...)
    {
        TakeDeviceControl();

        SubscribeToDeviceStateChange([this](auto state) {
            auto shutdown = GetProperty<bool>("please-shut-me-down", false);
            try {
                switch (state) {
                    case DeviceState::Idle: {
                        ChangeDeviceState(shutdown ? DeviceStateTransition::End
                                                    : DeviceStateTransition::InitDevice);
                        break;
                    }
                    case DeviceState::InitializingDevice: {
                        ChangeDeviceState(DeviceStateTransition::CompleteInit);
                        break;
                    }
                    case DeviceState::Initialized: {
                        ChangeDeviceState(DeviceStateTransition::Bind);
                        break;
                    }
                    case DeviceState::Bound: {
                        ChangeDeviceState(DeviceStateTransition::Connect);
                        break;
                    }
                    case DeviceState::DeviceReady: {
                        ChangeDeviceState(shutdown ? DeviceStateTransition::ResetDevice
                                                    : DeviceStateTransition::InitTask);
                        break;
                    }
                    case DeviceState::Ready: {
                        ChangeDeviceState(shutdown ? DeviceStateTransition::ResetTask
                                                    : DeviceStateTransition::Run);
                        break;
                    }
                    case DeviceState::Running: {
                        ChangeDeviceState(DeviceStateTransition::Stop);
                        break;
                    }
                    case DeviceState::Exiting: {
                        ReleaseDeviceControl();
                        break;
                    }
                    default:
                        break;
                }
            } catch (fair::mq::PluginServices::DeviceControlError const&) {
                // this means we do not have device control
            }
        });
    }

    ~MyController() override { ReleaseDeviceControl(); }
};

// auto MyControllerProgramOptions() -> fair::mq::Plugin::ProgOptions
// {
// auto plugin_options = boost::program_options::options_description{"MyController Plugin"};
// plugin_options.add_options()
// ("custom-dummy-option", boost::program_options::value<std::string>(), "Cool custom option.")
// ("custom-dummy-option2", boost::program_options::value<std::string>(), "Another one.");
// return plugin_options;
// }

}   // namespace example

REGISTER_FAIRMQ_PLUGIN(example::MyController,   // Class name
                       mycontroller,            // Plugin name (string, lower case chars only)
                       (fair::mq::Plugin::Version{0, 42, 0}),     // Version
                       "Mr. Dummy <dummy@test.net>",              // Maintainer
                       "https://git.test.net/mycontroller.git",   // Homepage
                       //                       example::MyControllerProgramOptions   // Free
                       //                       function which declares custom
                       // program options for the plugin
                       fair::mq::Plugin::NoProgramOptions)

#endif /* FAIR_MQ_EXAMPLE_CUSTOM_CONTROLLER_PLUGIN */
