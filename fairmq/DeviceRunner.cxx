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

using namespace std;
using namespace fair::mq;

DeviceRunner::DeviceRunner(int argc, char* const argv[], bool printLogo)
    : fRawCmdLineArgs(tools::ToStrVector(argc, argv, false))
    , fConfig()
    , fDevice(nullptr)
    , fPluginManager(fRawCmdLineArgs)
    , fPrintLogo(printLogo)
    , fEvents()
{}

bool DeviceRunner::HandleGeneralOptions(const fair::mq::ProgOptions& config, bool printLogo)
{
    if (config.Count("help")) {
        config.PrintHelp();
        return false;
    }

    if (config.Count("print-options")) {
        config.PrintOptionsRaw();
        return false;
    }

    if (config.Count("print-channels") || config.Count("version")) {
        fair::Logger::SetConsoleSeverity("nolog");
    } else {
        string severity = config.GetProperty<string>("severity");
        string logFile = config.GetProperty<string>("log-to-file");
        string logFileSeverity = config.GetProperty<string>("file-severity");
        bool color = config.GetProperty<bool>("color");

        string verbosity = config.GetProperty<string>("verbosity");
        fair::Logger::SetVerbosity(verbosity);

        if (logFile != "") {
            fair::Logger::InitFileSink(logFileSeverity, logFile);
            fair::Logger::SetConsoleSeverity("nolog");
        } else {
            fair::Logger::SetConsoleColor(color);
            fair::Logger::SetConsoleSeverity(severity);
        }

        if (printLogo) {
            LOG(info) << endl
                << "      ______      _    _______  _________ " << endl
                << "     / ____/___ _(_)_______   |/  /_  __ \\    version " << FAIRMQ_GIT_VERSION << endl
                << "    / /_  / __ `/ / ___/__  /|_/ /_  / / /    build " << FAIRMQ_BUILD_TYPE << endl
                << "   / __/ / /_/ / / /    _  /  / / / /_/ /     " << FAIRMQ_REPO_URL << endl
                << "  /_/    \\__,_/_/_/     /_/  /_/  \\___\\_\\     " << FAIRMQ_LICENSE << "  © " << FAIRMQ_COPYRIGHT << endl;
        }

        config.PrintOptions();
    }

    return true;
}

void DeviceRunner::SubscribeForConfigChange()
{
    fConfig.Subscribe<bool>("device-runner", [](const std::string& key, const bool val) {
        if (key == "color") {
            fair::Logger::SetConsoleColor(val);
        }
    });
    fConfig.Subscribe<string>("device-runner", [&](const std::string& key, const std::string val) {
        if (key == "severity") {
            fair::Logger::SetConsoleSeverity(val);
        } else if (key == "file-severity") {
            fair::Logger::SetFileSeverity(val);
        } else if (key == "verbosity") {
            fair::Logger::SetVerbosity(val);
        } else if (key == "log-to-file") {
            string fileSeverity = fConfig.GetProperty<string>("file-severity");
            fair::Logger::InitFileSink(fileSeverity, val);
        }
    });
}
void DeviceRunner::UnsubscribeFromConfigChange()
{
    fConfig.Unsubscribe<bool>("device-runner");
    fConfig.Unsubscribe<string>("device-runner");
}

auto DeviceRunner::Run() -> int
{
    fPluginManager.LoadPlugin("s:config");

    ////// CALL HOOK ///////
    fEvents.Emit<hooks::LoadPlugins>(*this);
    ////////////////////////

    // Load builtin plugins last
    fPluginManager.LoadPlugin("s:control");

    ////// CALL HOOK ///////
    fEvents.Emit<hooks::SetCustomCmdLineOptions>(*this);
    ////////////////////////

    fPluginManager.ForEachPluginProgOptions([&](boost::program_options::options_description options) {
        fConfig.AddToCmdLineOptions(options);
    });
    fConfig.AddToCmdLineOptions(fPluginManager.ProgramOptions());

    ////// CALL HOOK ///////
    fEvents.Emit<hooks::ModifyRawCmdLineArgs>(*this);
    ////////////////////////

    fConfig.ParseAll(fRawCmdLineArgs, true);

    if (!HandleGeneralOptions(fConfig)) {
        return 0;
    }

    fConfig.Notify();

    // handle configuration updates (for general options)
    SubscribeForConfigChange();

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
        cout << "FairMQ version: " << FAIRMQ_GIT_VERSION << endl;
        cout << "User device version: " << fDevice->GetVersion() << endl;
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

    // stop handling configuration updates (for general options)
    UnsubscribeFromConfigChange();

    return 0;
}

auto DeviceRunner::RunWithExceptionHandlers() -> int
{
    try {
        return Run();
    } catch (exception& e) {
        LOG(error) << "Uncaught exception reached the top of DeviceRunner: " << e.what();
        return 1;
    } catch (...) {
        LOG(error) << "Uncaught exception reached the top of DeviceRunner.";
        return 1;
    }
}
