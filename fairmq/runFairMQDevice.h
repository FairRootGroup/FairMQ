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
#include <tools/runSimpleMQStateMachine.h>
#include <fairmq/PluginManager.h>
#include <boost/program_options.hpp>
#include <memory>

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
        boost::program_options::options_description customOptions("Custom options");
        addCustomOptions(customOptions);

        // Plugin manager needs to be destroyed after config !
        // TODO Investigate, why
        auto pluginManager = fair::mq::PluginManager::MakeFromCommandLineOptions(fair::mq::tools::ToStrVector(argc, argv));
        FairMQProgOptions config;
        config.AddToCmdLineOptions(customOptions);

        pluginManager->ForEachPluginProgOptions([&config](boost::program_options::options_description options){
            config.AddToCmdLineOptions(options);
        });
        config.AddToCmdLineOptions(pluginManager->ProgramOptions());

        config.ParseAll(argc, argv, true);

        std::shared_ptr<FairMQDevice> device{getDevice(config)};
        if (!device)
        {
            LOG(ERROR) << "getDevice(): no valid device provided. Exiting.";
            return 1;
        }

        pluginManager->EmplacePluginServices(&config, device);
        pluginManager->InstantiatePlugins();

        int result = runStateMachine(*device, config);

        if (config.Count("version"))
        {
            pluginManager->ForEachPlugin([](fair::mq::Plugin& plugin){ std::cout << "plugin: " << plugin << std::endl; });
        }

        if (result > 0)
        {
            return 1;
        }
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
