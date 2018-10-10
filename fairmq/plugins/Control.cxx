/********************************************************************************
 * Copyright (C) 2017-2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "Control.h"

#include <termios.h> // for the interactive mode
#include <poll.h> // for the interactive mode
#include <csignal> // catching system signals
#include <cstdlib>
#include <functional>
#include <atomic>

using namespace std;

namespace
{
    std::atomic<sig_atomic_t> gLastSignal(0);
    std::atomic<int> gSignalCount(0);

    extern "C" auto signal_handler(int signal) -> void
    {
        ++gSignalCount;
        gLastSignal = signal;

        if (gSignalCount > 1)
        {
            std::abort();
        }
    }
}

namespace fair
{
namespace mq
{
namespace plugins
{

Control::Control(const string& name, const Plugin::Version version, const string& maintainer, const string& homepage, PluginServices* pluginServices)
    : Plugin(name, version, maintainer, homepage, pluginServices)
    , fControllerThread()
    , fSignalHandlerThread()
    , fEvents()
    , fEventsMutex()
    , fControllerMutex()
    , fNewEvent()
    , fDeviceShutdownRequested(false)
    , fDeviceHasShutdown(false)
    , fPluginShutdownRequested(false)
{
    SubscribeToDeviceStateChange([&](DeviceState newState)
    {
        {
            lock_guard<mutex> lock{fEventsMutex};
            fEvents.push(newState);
        }
        fNewEvent.notify_one();
    });

    try
    {
        TakeDeviceControl();

        auto control = GetProperty<string>("control");

        if (control == "static")
        {
            LOG(debug) << "Running builtin controller: static";
            fControllerThread = thread(&Control::StaticMode, this);
        }
        else if (control == "interactive")
        {
            LOG(debug) << "Running builtin controller: interactive";
            fControllerThread = thread(&Control::InteractiveMode, this);
        }
        else
        {
            LOG(error) << "Unrecognized control mode '" << control << "' requested. " << "Ignoring and falling back to static control mode.";
            fControllerThread = thread(&Control::StaticMode, this);
        }
    }
    catch (PluginServices::DeviceControlError& e)
    {
        // If we are here, it means another plugin has taken control. That's fine, just print the exception message and do nothing else.
        LOG(debug) << e.what();
    }

    LOG(debug) << "catch-signals: " << GetProperty<int>("catch-signals");
    if (GetProperty<int>("catch-signals") > 0)
    {
        fSignalHandlerThread = thread(&Control::SignalHandler, this);
        signal(SIGINT, signal_handler);
        signal(SIGTERM, signal_handler);
    }
    else
    {
        LOG(warn) << "Signal handling (e.g. Ctrl-C) has been deactivated.";
    }
}

auto ControlPluginProgramOptions() -> Plugin::ProgOptions
{
    namespace po = boost::program_options;
    auto pluginOptions = po::options_description{"Control (builtin) Plugin"};
    pluginOptions.add_options()
        ("control",       po::value<string>()->default_value("interactive"), "Control mode, 'static' or 'interactive'")
        ("catch-signals", po::value<int   >()->default_value(1),             "Enable signal handling (1/0).");
    return pluginOptions;
}

auto Control::InteractiveMode() -> void
try
{
    RunStartupSequence();

    char input; // hold the user console input
    pollfd cinfd[1];
    cinfd[0].fd = fileno(stdin);
    cinfd[0].events = POLLIN;

    struct termios t;
    tcgetattr(STDIN_FILENO, &t); // get the current terminal I/O structure
    t.c_lflag &= ~ICANON; // disable canonical input
    t.c_lflag &= ~ECHO; // do not echo input chars
    tcsetattr(STDIN_FILENO, TCSANOW, &t); // apply the new settings

    PrintInteractiveHelp();

    bool keepRunning = true;

    while (keepRunning)
    {
        if (poll(cinfd, 1, 500))
        {
            if (fDeviceShutdownRequested)
            {
                break;
            }

            cin >> input;

            switch (input)
            {
                case 'i':
                    LOG(info) << "\n\n --> [i] init device\n";
                    ChangeDeviceState(DeviceStateTransition::InitDevice);
                break;
                case 'j':
                    LOG(info) << "\n\n --> [j] init task\n";
                    ChangeDeviceState(DeviceStateTransition::InitTask);
                break;
                case 'p':
                    LOG(info) << "\n\n --> [p] pause\n";
                    ChangeDeviceState(DeviceStateTransition::Pause);
                break;
                case 'r':
                    LOG(info) << "\n\n --> [r] run\n";
                    ChangeDeviceState(DeviceStateTransition::Run);
                break;
                case 's':
                    LOG(info) << "\n\n --> [s] stop\n";
                    ChangeDeviceState(DeviceStateTransition::Stop);
                break;
                case 't':
                    LOG(info) << "\n\n --> [t] reset task\n";
                    ChangeDeviceState(DeviceStateTransition::ResetTask);
                break;
                case 'd':
                    LOG(info) << "\n\n --> [d] reset device\n";
                    ChangeDeviceState(DeviceStateTransition::ResetDevice);
                break;
                case 'k':
                    LOG(info) << "\n\n --> [k] increase log severity\n";
                    CycleLogConsoleSeverityUp();
                break;
                case 'l':
                    LOG(info) << "\n\n --> [l] decrease log severity\n";
                    CycleLogConsoleSeverityDown();
                break;
                case 'n':
                    LOG(info) << "\n\n --> [n] increase log verbosity\n";
                    CycleLogVerbosityUp();
                break;
                case 'm':
                    LOG(info) << "\n\n --> [m] decrease log verbosity\n";
                    CycleLogVerbosityDown();
                break;
                case 'h':
                    LOG(info) << "\n\n --> [h] help\n";
                    PrintInteractiveHelp();
                break;
                // case 'x':
                //     LOG(info) << "\n\n --> [x] ERROR\n";
                //     ChangeDeviceState(DeviceStateTransition::ERROR_FOUND);
                //     break;
                case 'q':
                    LOG(info) << "\n\n --> [q] end\n";
                    keepRunning = false;
                break;
                default:
                    LOG(info) << "Invalid input: [" << input << "]";
                    PrintInteractiveHelp();
                break;
            }
        }

        if (GetCurrentDeviceState() == DeviceState::Error)
        {
            throw DeviceErrorState("Controlled device transitioned to error state.");
        }

        if (fDeviceShutdownRequested)
        {
            break;
        }
    }

    tcgetattr(STDIN_FILENO, &t); // get the current terminal I/O structure
    t.c_lflag |= ICANON; // re-enable canonical input
    t.c_lflag |= ECHO; // echo input chars
    tcsetattr(STDIN_FILENO, TCSANOW, &t); // apply the new settings

    RunShutdownSequence();
}
catch (PluginServices::DeviceControlError& e)
{
    // If we are here, it means another plugin has taken control. That's fine, just print the exception message and do nothing else.
    LOG(debug) << e.what();
}
catch (DeviceErrorState&)
{
}

auto Control::PrintInteractiveHelp() -> void
{
    stringstream ss;
    ss << "\nFollowing control commands are available:\n\n"
       << "[h] help, [p] pause, [r] run, [s] stop, [t] reset task, [d] reset device, [q] end, [j] init task, [i] init device\n"
       << "[k] increase log severity [l] decrease log severity [n] increase log verbosity [m] decrease log verbosity\n\n";
    cout << ss.str() << flush;
}

auto Control::WaitForNextState() -> DeviceState
{
    unique_lock<mutex> lock{fEventsMutex};
    while (fEvents.empty())
    {
        fNewEvent.wait_for(lock, chrono::milliseconds(50));
    }

    auto result = fEvents.front();

    if (result == DeviceState::Error)
    {
        throw DeviceErrorState("Controlled device transitioned to error state.");
    }

    fEvents.pop();

    return result;
}

auto Control::StaticMode() -> void
try
{
    RunStartupSequence();

    {
        // Wait for next state, which is DeviceState::Ready,
        // or for device shutdown request (Ctrl-C)
        unique_lock<mutex> lock{fEventsMutex};
        while (fEvents.empty() && !fDeviceShutdownRequested)
        {
            fNewEvent.wait_for(lock, chrono::milliseconds(50));
        }

        if (fEvents.front() == DeviceState::Error)
        {
            throw DeviceErrorState("Controlled device transitioned to error state.");
        }
    }

    RunShutdownSequence();
}
catch (PluginServices::DeviceControlError& e)
{
    // If we are here, it means another plugin has taken control. That's fine, just print the exception message and do nothing else.
    LOG(debug) << e.what();
}
catch (DeviceErrorState&)
{
}

auto Control::SignalHandler() -> void
{
    while (gSignalCount == 0 && !fPluginShutdownRequested)
    {
        this_thread::sleep_for(chrono::milliseconds(100));
    }

    if (!fPluginShutdownRequested)
    {
        LOG(info) << "Received device shutdown request (signal " << gLastSignal << ").";
        LOG(info) << "Waiting for graceful device shutdown. Hit Ctrl-C again to abort immediately.";

        // Signal and wait for controller thread, if we are controller
        fDeviceShutdownRequested = true;
        {
            unique_lock<mutex> lock(fControllerMutex);
            if (fControllerThread.joinable()) fControllerThread.join();
        }

        if (!fDeviceHasShutdown)
        {
            // Take over control and attempt graceful shutdown
            StealDeviceControl();
            try
            {
                RunShutdownSequence();
            }
            catch (PluginServices::DeviceControlError& e)
            {
                LOG(info) << "Graceful device shutdown failed: " << e.what() << " If hanging, hit Ctrl-C again to abort immediately.";
            }
            catch (...)
            {
                LOG(info) << "Graceful device shutdown failed. If hanging, hit Ctrl-C again to abort immediately.";
            }
        }
    }
}

auto Control::RunShutdownSequence() -> void
{
    auto nextState = GetCurrentDeviceState();
    EmptyEventQueue();
    while (nextState != DeviceState::Exiting && nextState != DeviceState::Error)
    {
        switch (nextState)
        {
            case DeviceState::Idle:
                ChangeDeviceState(DeviceStateTransition::End);
                break;
            case DeviceState::DeviceReady:
                ChangeDeviceState(DeviceStateTransition::ResetDevice);
                break;
            case DeviceState::Ready:
                ChangeDeviceState(DeviceStateTransition::ResetTask);
                break;
            case DeviceState::Running:
                ChangeDeviceState(DeviceStateTransition::Stop);
                break;
            case DeviceState::Paused:
                ChangeDeviceState(DeviceStateTransition::Resume);
                break;
            default:
                LOG(debug) << "Controller ignoring event: " << nextState;
                break;
        }

        nextState = WaitForNextState();
    }

    fDeviceHasShutdown = true;
    ReleaseDeviceControl();
}

auto Control::RunStartupSequence() -> void
{
    ChangeDeviceState(DeviceStateTransition::InitDevice);
    while (WaitForNextState() != DeviceState::DeviceReady) {}
    ChangeDeviceState(DeviceStateTransition::InitTask);
    while (WaitForNextState() != DeviceState::Ready) {}
    ChangeDeviceState(DeviceStateTransition::Run);
    while (WaitForNextState() != DeviceState::Running) {}
}

auto Control::EmptyEventQueue() -> void
{
    lock_guard<mutex> lock{fEventsMutex};
    fEvents = queue<DeviceState>{};
}

Control::~Control()
{
    // Notify threads to exit
    fPluginShutdownRequested = true;

    {
        unique_lock<mutex> lock(fControllerMutex);
        if (fControllerThread.joinable()) fControllerThread.join();
    }
    if (fSignalHandlerThread.joinable()) fSignalHandlerThread.join();

    UnsubscribeFromDeviceStateChange();
}

} /* namespace plugins */
} /* namespace mq */
} /* namespace fair */
