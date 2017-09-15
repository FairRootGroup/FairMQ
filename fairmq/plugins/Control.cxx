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

using namespace std;

namespace fair
{
namespace mq
{
namespace plugins
{

Control::Control(const string name, const Plugin::Version version, const string maintainer, const string homepage, PluginServices* pluginServices)
    : Plugin(name, version, maintainer, homepage, pluginServices)
    , fControllerThread()
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
            LOG(ERROR) << "Unrecognized control mode '" << control << "' requested via command line. " << "Ignoring and falling back to static control mode.";
            fControllerThread = thread(&Control::StaticMode, this);
        }
    }
    catch (PluginServices::DeviceControlError& e)
    {
        LOG(DEBUG) << e.what();
    }
}

auto ControlPluginProgramOptions() -> Plugin::ProgOptions
{
    auto pluginOptions = boost::program_options::options_description{"Control (builtin) Plugin"};
    pluginOptions.add_options()
        ("ctrlmode", boost::program_options::value<string>(), "Control mode, 'static' or 'interactive'");
        // should rename to --control and remove control from device options ?
    return pluginOptions;
}

auto Control::InteractiveMode() -> void
{
    SubscribeToDeviceStateChange([&](DeviceState newState)
    {
        {
            lock_guard<mutex> lock{fEventsMutex};
            fEvents.push(newState);
        }
        fNewEvent.notify_one();
    });

    ChangeDeviceState(DeviceStateTransition::InitDevice);
    while (WaitForNextState() != DeviceState::DeviceReady) {}
    ChangeDeviceState(DeviceStateTransition::InitTask);
    while (WaitForNextState() != DeviceState::Ready) {}
    ChangeDeviceState(DeviceStateTransition::Run);

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
            if (DeviceTerminated())
            {
                keepRunning = false;
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

                    ChangeDeviceState(DeviceStateTransition::Stop);

                    ChangeDeviceState(DeviceStateTransition::ResetTask);
                    while (WaitForNextState() != DeviceState::DeviceReady) {}

                    ChangeDeviceState(DeviceStateTransition::ResetDevice);
                    while (WaitForNextState() != DeviceState::Idle) {}

                    ChangeDeviceState(DeviceStateTransition::End);

                    if (GetCurrentDeviceState() == DeviceState::Exiting)
                    {
                        keepRunning = false;
                    }

                    LOG(INFO) << "Exiting.";
                break;
                default:
                    LOG(INFO) << "Invalid input: [" << input << "]";
                    PrintInteractiveHelp();
                break;
            }
        }

        if (DeviceTerminated())
        {
            keepRunning = false;
        }
    }

    tcgetattr(STDIN_FILENO, &t); // get the current terminal I/O structure
    t.c_lflag |= ICANON; // re-enable canonical input
    tcsetattr(STDIN_FILENO, TCSANOW, &t); // apply the new settings

    UnsubscribeFromDeviceStateChange();
    ReleaseDeviceControl();
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
    SubscribeToDeviceStateChange([&](DeviceState newState)
    {
        {
            lock_guard<mutex> lock{fEventsMutex};
            fEvents.push(newState);
        }
        fNewEvent.notify_one();
    });

    ChangeDeviceState(DeviceStateTransition::InitDevice);
    while (WaitForNextState() != DeviceState::DeviceReady) {}

    ChangeDeviceState(DeviceStateTransition::InitTask);
    while (WaitForNextState() != DeviceState::Ready) {}
    ChangeDeviceState(DeviceStateTransition::Run);
    while (WaitForNextState() != DeviceState::Ready) {}
    if (!DeviceTerminated())
    {
        ChangeDeviceState(DeviceStateTransition::ResetTask);
        while (WaitForNextState() != DeviceState::DeviceReady) {}
        ChangeDeviceState(DeviceStateTransition::ResetDevice);
        while (WaitForNextState() != DeviceState::Idle) {}
        ChangeDeviceState(DeviceStateTransition::End);
        while (WaitForNextState() != DeviceState::Exiting) {}
    }

    UnsubscribeFromDeviceStateChange();
    ReleaseDeviceControl();
}

Control::~Control()
{
    if (fControllerThread.joinable())
    {
        fControllerThread.join();
    }
}

} /* namespace plugins */
} /* namespace mq */
} /* namespace fair */
