/********************************************************************************
 *    Copyright (C) 2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "Control.h"

#include <termios.h> // for the interactive mode
#include <poll.h> // for the interactive mode
#include <csignal> // catching system signals
#include <functional>

using namespace std;

namespace
{
    std::function<void(int)> gSignalHandlerClosure;

    extern "C" auto signal_handler(int signal) -> void
    {
        gSignalHandlerClosure(signal);
    }
}

namespace fair
{
namespace mq
{
namespace plugins
{

Control::Control(const string name, const Plugin::Version version, const string maintainer, const string homepage, PluginServices* pluginServices)
    : Plugin(name, version, maintainer, homepage, pluginServices)
    , fControllerThread()
    , fSignalHandlerThread()
    , fDeviceTerminationRequested{false}
    , fEvents()
    , fEventsMutex()
    , fNewEvent()
{
    try
    {
        TakeDeviceControl();

        auto control = GetProperty<string>("control");

        if (control == "static")
        {
            LOG(DEBUG) << "Running builtin controller: static";
            fControllerThread = thread(&Control::StaticMode, this);
        }
        else if (control == "interactive")
        {
            LOG(DEBUG) << "Running builtin controller: interactive";
            fControllerThread = thread(&Control::InteractiveMode, this);
        }
        else
        {
            LOG(ERROR) << "Unrecognized control mode '" << control << "' requested. " << "Ignoring and falling back to static control mode.";
            fControllerThread = thread(&Control::StaticMode, this);
        }

        LOG(DEBUG) << "catch-signals: " << GetProperty<int>("catch-signals");
        if (GetProperty<int>("catch-signals") > 0)
        {
            gSignalHandlerClosure = bind(&Control::SignalHandler, this, placeholders::_1);
            signal(SIGINT, signal_handler);
            signal(SIGTERM, signal_handler);
        }
        else
        {
            LOG(WARN) << "Signal handling (e.g. Ctrl-C) has been deactivated.";
        }
    }
    catch (PluginServices::DeviceControlError& e)
    {
        // If we are here, it means another plugin has taken control. That's fine, just print the exception message and do nothing else.
        LOG(DEBUG) << e.what();
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
{
    try
    {
        SubscribeToDeviceStateChange([&](DeviceState newState)
        {
            {
                lock_guard<mutex> lock{fEventsMutex};
                fEvents.push(newState);
            }
            fNewEvent.notify_one();
        });

        RunStartupSequence();

        char input; // hold the user console input
        pollfd cinfd[1];
        cinfd[0].fd = fileno(stdin);
        cinfd[0].events = POLLIN;

        struct termios t;
        tcgetattr(STDIN_FILENO, &t); // get the current terminal I/O structure
        t.c_lflag &= ~ICANON; // disable canonical input
        tcsetattr(STDIN_FILENO, TCSANOW, &t); // apply the new settings

        PrintInteractiveHelp();

        bool keepRunning = true;

        while (keepRunning)
        {
            if (poll(cinfd, 1, 500))
            {
                if (fDeviceTerminationRequested)
                {
                    break;
                }

                cin >> input;

                switch (input)
                {
                    case 'i':
                        LOG(INFO) << "[i] init device";
                        ChangeDeviceState(DeviceStateTransition::InitDevice);
                    break;
                    case 'j':
                        LOG(INFO) << "[j] init task";
                        ChangeDeviceState(DeviceStateTransition::InitTask);
                    break;
                    case 'p':
                        LOG(INFO) << "[p] pause";
                        ChangeDeviceState(DeviceStateTransition::Pause);
                    break;
                    case 'r':
                        LOG(INFO) << "[r] run";
                        ChangeDeviceState(DeviceStateTransition::Run);
                    break;
                    case 's':
                        LOG(INFO) << "[s] stop";
                        ChangeDeviceState(DeviceStateTransition::Stop);
                    break;
                    case 't':
                        LOG(INFO) << "[t] reset task";
                        ChangeDeviceState(DeviceStateTransition::ResetTask);
                    break;
                    case 'd':
                        LOG(INFO) << "[d] reset device";
                        ChangeDeviceState(DeviceStateTransition::ResetDevice);
                    break;
                    case 'h':
                        LOG(INFO) << "[h] help";
                        PrintInteractiveHelp();
                    break;
                    // case 'x':
                    //     LOG(INFO) << "[x] ERROR";
                    //     ChangeDeviceState(DeviceStateTransition::ERROR_FOUND);
                    //     break;
                    case 'q':
                        LOG(INFO) << "[q] end";
                        keepRunning = false;
                    break;
                    default:
                        LOG(INFO) << "Invalid input: [" << input << "]";
                        PrintInteractiveHelp();
                    break;
                }
            }

            if (fDeviceTerminationRequested)
            {
                keepRunning = false;
            }
        }

        tcgetattr(STDIN_FILENO, &t); // get the current terminal I/O structure
        t.c_lflag |= ICANON; // re-enable canonical input
        tcsetattr(STDIN_FILENO, TCSANOW, &t); // apply the new settings

        if (!fDeviceTerminationRequested)
        {
            RunShutdownSequence();
        }
    }
    catch (PluginServices::DeviceControlError& e)
    {
        // If we are here, it means another plugin has taken control. That's fine, just print the exception message and do nothing else.
        LOG(DEBUG) << e.what();
    }
}

auto Control::PrintInteractiveHelp() -> void
{
    LOG(INFO) << "Use keys to control the state machine:";
    LOG(INFO) << "[h] help, [p] pause, [r] run, [s] stop, [t] reset task, [d] reset device, [q] end, [j] init task, [i] init device";
}

auto Control::WaitForNextState() -> DeviceState
{
    unique_lock<mutex> lock{fEventsMutex};
    while (fEvents.empty())
    {
        fNewEvent.wait(lock);
    }

    auto result = fEvents.front();
    fEvents.pop();
    return result;
}

auto Control::StaticMode() -> void
{
    try
    {
        SubscribeToDeviceStateChange([&](DeviceState newState)
        {
            {
                lock_guard<mutex> lock{fEventsMutex};
                fEvents.push(newState);
            }
            fNewEvent.notify_one();
        });

        RunStartupSequence();
    
        {
            // Wait for next state, which is DeviceState::Ready,
            // or for device termination request
            unique_lock<mutex> lock{fEventsMutex};
            while (fEvents.empty() && !fDeviceTerminationRequested)
            {
                fNewEvent.wait(lock);
            }
        }

        if (!fDeviceTerminationRequested)
        {
            RunShutdownSequence();
        }
    }
    catch (PluginServices::DeviceControlError& e)
    {
        // If we are here, it means another plugin has taken control. That's fine, just print the exception message and do nothing else.
        LOG(DEBUG) << e.what();
    }
}

auto Control::SignalHandler(int signal) -> void
{

    if (!fDeviceTerminationRequested)
    {
        fDeviceTerminationRequested = true;

        StealDeviceControl();

        LOG(INFO) << "Received device shutdown request (signal " << signal << ").";
        LOG(INFO) << "Waiting for graceful device shutdown. Hit Ctrl-C again to abort immediately.";

        fSignalHandlerThread = thread(&Control::RunShutdownSequence, this);
    }
    else
    {
        LOG(WARN) << "Received 2nd device shutdown request (signal " << signal << ").";
        LOG(WARN) << "Aborting immediately !";
        abort();
    }
}

auto Control::RunShutdownSequence() -> void
{
    auto nextState = GetCurrentDeviceState();
    EmptyEventQueue();
    while (nextState != DeviceState::Exiting)
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
                break;
        }

        nextState = WaitForNextState();
    }

    UnsubscribeFromDeviceStateChange();
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
    if (fControllerThread.joinable()) fControllerThread.join();
    if (fSignalHandlerThread.joinable()) fSignalHandlerThread.join();
}

} /* namespace plugins */
} /* namespace mq */
} /* namespace fair */
