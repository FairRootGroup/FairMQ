/********************************************************************************
 *    Copyright (C) 2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <FairMQLogger.h>
#include <options/FairMQProgOptions.h>
#include <FairMQDevice.h>
#include <fairmq/PluginManager.h>
#include <fairmq/Tools.h>
#include <boost/program_options.hpp>
#include <memory>
#include <string>
#include <iostream>

template <typename R>
class GenericFairMQDevice : public FairMQDevice
{
  public:
    GenericFairMQDevice(R func) : r(func) {}

  protected:
    virtual bool ConditionalRun() { return r(*static_cast<FairMQDevice*>(this)); }

  private:
    R r;
};

template <typename R>
FairMQDevice* makeDeviceWithConditionalRun(R r)
{
    return new GenericFairMQDevice<R>(r);
}

using FairMQDevicePtr = FairMQDevice*;

// to be implemented by the user to return a child class of FairMQDevice
FairMQDevicePtr getDevice(const FairMQProgOptions& config);

// to be implemented by the user to add custom command line options (or just with empty body)
void addCustomOptions(boost::program_options::options_description&);

int main(int argc, const char** argv)
{
    try
    {
        // Call custom program options hook
        boost::program_options::options_description customOptions("Custom options");
        addCustomOptions(customOptions);

        // Create plugin manager and load command line supplied plugins
        // Plugin manager needs to be destroyed after config! TODO Investigate why
        auto pluginManager = fair::mq::PluginManager::MakeFromCommandLineOptions(fair::mq::tools::ToStrVector(argc, argv));

        // Load builtin plugins last
        pluginManager->LoadPlugin("s:control");

        // Construct command line options parser
        FairMQProgOptions config;
        config.AddToCmdLineOptions(customOptions);
        pluginManager->ForEachPluginProgOptions([&config](boost::program_options::options_description options){
            config.AddToCmdLineOptions(options);
        });
        config.AddToCmdLineOptions(pluginManager->ProgramOptions());

        // Parse command line options
        config.ParseAll(argc, argv, true);

        // Call device creation hook
        std::shared_ptr<FairMQDevice> device{getDevice(config)};
        if (!device)
        {
            LOG(ERROR) << "getDevice(): no valid device provided. Exiting.";
            return 1;
        }

        // Handle --print-channels
        device->RegisterChannelEndpoints();
        if (config.Count("print-channels"))
        {
            device->PrintRegisteredChannels();
            device->ChangeState(FairMQDevice::END);
            return 0;
        }

        // Handle --version
        if (config.Count("version"))
        {
            std::cout << "User device version: " << device->GetVersion() << std::endl;
            std::cout << "FAIRMQ_INTERFACE_VERSION: " << FAIRMQ_INTERFACE_VERSION << std::endl;
            device->ChangeState(FairMQDevice::END);
            return 0;
        }

        LOG(DEBUG) << "PID: " << getpid();

        // Configure device
        device->SetConfig(config);

        // Initialize plugin services
        pluginManager->EmplacePluginServices(&config, device);

        // Instantiate and run plugins
        pluginManager->InstantiatePlugins();

        // Wait for control plugin to release device control
        pluginManager->WaitForPluginsToReleaseDeviceControl();
    }
    catch (std::exception& e)
    {
        LOG(ERROR) << "Unhandled exception reached the top of main: " << e.what() << ", application will now exit";
        return 1;
    }
    catch (...)
    {
        LOG(ERROR) << "Non-exception instance being thrown. Please make sure you use std::runtime_exception() instead. Application will now exit.";
        return 1;
    }

    return 0;
}
