/********************************************************************************
 *    Copyright (C) 2022 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "MyController.h"
#include "MyDevice.h"

#include <chrono>   // for std::chrono_literals
#include <fairlogger/Logger.h>
#include <fairmq/DeviceRunner.h>
#include <memory>   // for std::make_unique

int main(int argc, char* argv[])
{
    using namespace fair::mq;
    using namespace std::chrono_literals;

    DeviceRunner runner(argc, argv);

    runner.AddHook<hooks::LoadPlugins>([](DeviceRunner& r) {
        r.fPluginManager.LoadPlugin("s:mycontroller");
        // 's:' stands for static because the plugin is compiled into the executable
        // 'mycontroller' is the plugin name passed as second arg to REGISTER_FAIRMQ_PLUGIN
    });

    fair::Logger::SetConsoleSeverity(fair::Severity::debug);

    runner.AddHook<hooks::InstantiateDevice>([](DeviceRunner& r) {
        r.fConfig.SetProperty<int>("catch-signals", 0);
        r.fConfig.SetProperty<bool>("please-shut-me-down", false);
        r.fDevice = std::make_unique<example::MyDevice>(r.fConfig);

        r.fDevice->SubscribeToStateChange("example", [&r](auto state) {
            if (state == State::Running) {
                r.fDevice->WaitFor(3s);
                r.fConfig.SetProperty<bool>("please-shut-me-down", true);
            }
        });
    });

    return runner.RunWithExceptionHandlers();
}
