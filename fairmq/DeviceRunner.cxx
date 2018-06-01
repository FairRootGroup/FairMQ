/********************************************************************************
 *    Copyright (C) 2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "DeviceRunner.h"
#include <fairmq/Tools.h>
#include <exception>

using namespace fair::mq;

DeviceRunner::DeviceRunner(int argc, char* const argv[])
: fRawCmdLineArgs{tools::ToStrVector(argc, argv, false)}
, fPluginManager{PluginManager::MakeFromCommandLineOptions(fRawCmdLineArgs)}
, fDevice{nullptr}
{
}

auto DeviceRunner::Run() -> int
{
    ////// CALL HOOK ///////
    fEvents.Emit<hooks::LoadPlugins>(*this);
    ////////////////////////

    // Load builtin plugins last
    fPluginManager->LoadPlugin("s:control");

    ////// CALL HOOK ///////
    fEvents.Emit<hooks::SetCustomCmdLineOptions>(*this);
    ////////////////////////

    fPluginManager->ForEachPluginProgOptions([&](boost::program_options::options_description options){
        fConfig.AddToCmdLineOptions(options);
    });
    fConfig.AddToCmdLineOptions(fPluginManager->ProgramOptions());

    ////// CALL HOOK ///////
    fEvents.Emit<hooks::ModifyRawCmdLineArgs>(*this);
    ////////////////////////

    if (fConfig.ParseAll(fRawCmdLineArgs, true))
    {
        return 0;
    }

    ////// CALL HOOK ///////
    fEvents.Emit<hooks::InstantiateDevice>(*this);
    ////////////////////////

    if (!fDevice)
    {
        LOG(error) << "getDevice(): no valid device provided. Exiting.";
        return 1;
    }

    fDevice->SetRawCmdLineArgs(fRawCmdLineArgs);

    // Handle --print-channels
    fDevice->RegisterChannelEndpoints();
    if (fConfig.Count("print-channels"))
    {
        fDevice->PrintRegisteredChannels();
        fDevice->ChangeState(FairMQDevice::END);
        return 0;
    }

    // Handle --version
    if (fConfig.Count("version"))
    {
        std::cout << "User device version: " << fDevice->GetVersion() << std::endl;
        std::cout << "FAIRMQ_INTERFACE_VERSION: " << FAIRMQ_INTERFACE_VERSION << std::endl;
        fDevice->ChangeState(FairMQDevice::END);
        return 0;
    }

    LOG(debug) << "PID: " << getpid();

    // Configure device
    fDevice->SetConfig(fConfig);

    // Initialize plugin services
    fPluginManager->EmplacePluginServices(&fConfig, fDevice);

    // Instantiate and run plugins
    fPluginManager->InstantiatePlugins();

    // Wait for control plugin to release device control
    fPluginManager->WaitForPluginsToReleaseDeviceControl();

    return 0;
}

auto DeviceRunner::RunWithExceptionHandlers() -> int
{
    try
    {
        return Run();
    }
    catch (std::exception& e)
    {
        LOG(error) << "Unhandled exception reached the top of main: " << e.what() << ", application will now exit";
        return 1;
    }
    catch (...)
    {
        LOG(error) << "Non-exception instance being thrown. Please make sure you use std::runtime_exception() instead. Application will now exit.";
        return 1;
    }
}
