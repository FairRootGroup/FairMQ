/********************************************************************************
 *    Copyright (C) 2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <fairmq/DeviceRunner.h>
#include <boost/program_options.hpp>
#include <memory>
#include <string>

using FairMQDevicePtr = FairMQDevice*;

// to be implemented by the user to return a child class of FairMQDevice
FairMQDevicePtr getDevice(const FairMQProgOptions& config);

// to be implemented by the user to add custom command line options (or just with empty body)
void addCustomOptions(boost::program_options::options_description&);

int main(int argc, char* argv[])
{
    using namespace fair::mq;
    using namespace fair::mq::hooks;

    try
    {
        fair::mq::DeviceRunner runner{argc, argv};

        // runner.AddHook<LoadPlugins>([](DeviceRunner& r){
        //     // for example:
        //     r.fPluginManager->SetSearchPaths({"/lib", "/lib/plugins"});
        //     r.fPluginManager->LoadPlugin("asdf");
        // });

        runner.AddHook<SetCustomCmdLineOptions>([](DeviceRunner& r){
            boost::program_options::options_description customOptions("Custom options");
            addCustomOptions(customOptions);
            r.fConfig.AddToCmdLineOptions(customOptions);
        });

        // runner.AddHook<ModifyRawCmdLineArgs>([](DeviceRunner& r){
        //     // for example:
        //     r.fRawCmdLineArgs.push_back("--blubb");
        // });

        runner.AddHook<InstantiateDevice>([](DeviceRunner& r){
            r.fDevice = std::unique_ptr<FairMQDevice>{getDevice(r.fConfig)};
        });

        return runner.Run();

        // Run with builtin catch all exception handler, just:
        // return runner.RunWithExceptionHandlers();
    }
    catch (std::exception& e)
    {
        LOG(error) << "Uncaught exception reached the top of main: " << e.what();
        return 1;
    }
    catch (...)
    {
        LOG(error) << "Uncaught exception reached the top of main.";
        return 1;
    }
}
