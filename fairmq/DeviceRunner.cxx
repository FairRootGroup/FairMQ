/********************************************************************************
 * Copyright (C) 2017-2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "DeviceRunner.h"

#include <exception>
#include <fairmq/Tools.h>
#include <fairmq/Version.h>

using namespace fair::mq;

DeviceRunner::DeviceRunner(int argc, char* const argv[], bool printLogo)
    : fRawCmdLineArgs(tools::ToStrVector(argc, argv, false))
    , fConfig()
    , fDevice(nullptr)
    , fPluginManager(fRawCmdLineArgs)
    , fPrintLogo(printLogo)
    , fEvents()
{}

auto DeviceRunner::Run() -> int
{
    ////// CALL HOOK ///////
    fEvents.Emit<hooks::LoadPlugins>(*this);
    ////////////////////////

    // Load builtin plugins last
    fPluginManager.LoadPlugin("s:control");

    ////// CALL HOOK ///////
    fEvents.Emit<hooks::SetCustomCmdLineOptions>(*this);
    ////////////////////////

    fPluginManager.ForEachPluginProgOptions(
        [&](boost::program_options::options_description options) {
            fConfig.AddToCmdLineOptions(options);
        });
    fConfig.AddToCmdLineOptions(fPluginManager.ProgramOptions());

    ////// CALL HOOK ///////
    fEvents.Emit<hooks::ModifyRawCmdLineArgs>(*this);
    ////////////////////////

    if (fConfig.ParseAll(fRawCmdLineArgs, true)) {
        return 0;
    }

    if (fPrintLogo) {
        LOG(info) << std::endl
                  << "      ______      _    _______  _________ " << std::endl
                  << "     / ____/___ _(_)_______   |/  /_  __ \\    version " << FAIRMQ_GIT_VERSION << std::endl
                  << "    / /_  / __ `/ / ___/__  /|_/ /_  / / /    build " << FAIRMQ_BUILD_TYPE << std::endl
                  << "   / __/ / /_/ / / /    _  /  / / / /_/ /     " << FAIRMQ_REPO_URL << std::endl
                  << "  /_/    \\__,_/_/_/     /_/  /_/  \\___\\_\\     " << FAIRMQ_LICENSE << "  © " << FAIRMQ_COPYRIGHT << std::endl;
    }

    ////// CALL HOOK ///////
    fEvents.Emit<hooks::InstantiateDevice>(*this);
    ////////////////////////

    if (!fDevice) {
        LOG(error) << "getDevice(): no valid device provided. Exiting.";
        return 1;
    }

    fDevice->SetRawCmdLineArgs(fRawCmdLineArgs);

    // Handle --print-channels
    fDevice->RegisterChannelEndpoints();
    if (fConfig.Count("print-channels")) {
        fDevice->PrintRegisteredChannels();
        fDevice->ChangeState(fair::mq::Transition::End);
        return 0;
    }

    // Handle --version
    if (fConfig.Count("version")) {
        std::cout << "User device version: " << fDevice->GetVersion() << std::endl;
        fDevice->ChangeState(fair::mq::Transition::End);
        return 0;
    }

    LOG(debug) << "PID: " << getpid();

    // Configure device
    fDevice->SetConfig(fConfig);

    // Initialize plugin services
    fPluginManager.EmplacePluginServices(fConfig, *fDevice);

    // Instantiate and run plugins
    fPluginManager.InstantiatePlugins();

    // Run the device
    fDevice->RunStateMachine();

    // Wait for control plugin to release device control
    fPluginManager.WaitForPluginsToReleaseDeviceControl();

    return 0;
}

auto DeviceRunner::RunWithExceptionHandlers() -> int
{
    try {
        return Run();
    } catch (std::exception& e) {
        LOG(error) << "Uncaught exception reached the top of DeviceRunner: " << e.what();
        return 1;
    } catch (...) {
        LOG(error) << "Uncaught exception reached the top of DeviceRunner.";
        return 1;
    }
}
