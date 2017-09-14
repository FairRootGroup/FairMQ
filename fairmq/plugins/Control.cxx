/********************************************************************************
 *    Copyright (C) 2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "Control.h"
#include <chrono>
#include <thread>


using namespace std;

namespace fair
{
namespace mq
{
namespace plugins
{

Control::Control(
    const string name,
    const Plugin::Version version,
    const string maintainer,
    const string homepage,
    PluginServices* pluginServices)
: Plugin(name, version, maintainer, homepage, pluginServices)
{
    try
    {
        TakeDeviceControl();

        auto control = GetProperty<string>("control");

        if(control == "static")
        {
            LOG(DEBUG) << "Running builtin controller: static";
            thread t(&Control::StaticMode, this);
            t.detach();
        }
        else if(control == "interactive")
        {
            LOG(DEBUG) << "Running builtin controller: interactive";
            thread t(&Control::InteractiveMode, this);
            t.detach();
        }
        else
        {
            LOG(ERROR) << "Unrecognized control mode '" << control << "' requested via command line. "
                << "Ignoring and falling back to interactive control mode.";
            thread t(&Control::InteractiveMode, this);
            t.detach();
        }
    }
    catch(PluginServices::DeviceControlError& e)
    {
        LOG(DEBUG) << e.what();
    }   
}

auto ControlPluginProgramOptions() -> Plugin::ProgOptions
{
    auto plugin_options = boost::program_options::options_description{"Control (builtin) Plugin"};
    plugin_options.add_options()
        ("ctrlmode", boost::program_options::value<string>(), "Control mode, 'static' or 'interactive'");
        // should rename to --control and remove control from device options ?
    return plugin_options;
}
    
auto Control::InteractiveMode() -> void
{
    LOG(ERROR) << "NOT YET IMPLEMENTED";
    ReleaseDeviceControl();
}

auto Control::WaitForNextState() -> DeviceState
{
    unique_lock<mutex> lock{fEventsMutex};
    while(fEvents.empty())
    {
        fNewEvent.wait(lock);
    }
    // lock.lock();
    auto result = fEvents.front();
    fEvents.pop();
    return result;
}

auto Control::StaticMode() -> void
{
    clock_t cStart = clock();
    auto tStart = chrono::high_resolution_clock::now();

    SubscribeToDeviceStateChange(
        [&](DeviceState newState){
            {
                lock_guard<mutex> lock{fEventsMutex};
                fEvents.push(newState);
            }
            fNewEvent.notify_one();
        }
    );
    
    ChangeDeviceState(DeviceStateTransition::InitDevice);
    while(WaitForNextState() != DeviceState::DeviceReady) {};
    
    clock_t cEnd = std::clock();
    auto tEnd = chrono::high_resolution_clock::now();

    LOG(DEBUG) << "Init time (CPU) : " << fixed << setprecision(2) << 1000.0 * (cEnd - cStart) / CLOCKS_PER_SEC << " ms";
    LOG(DEBUG) << "Init time (Wall): " << chrono::duration<double, milli>(tEnd - tStart).count() << " ms";
    
    ChangeDeviceState(DeviceStateTransition::InitTask);
    while(WaitForNextState() != DeviceState::Ready) {};
    ChangeDeviceState(DeviceStateTransition::Run);
    // WaitForNextState();
    // ChangeDeviceState(DeviceStateTransition::ResetTask);
    // WaitForNextState();
    // WaitForNextState();
    // ChangeDeviceState(DeviceStateTransition::ResetDevice);
    // WaitForNextState();
    // WaitForNextState();
    // ChangeDeviceState(DeviceStateTransition::End);
    while(WaitForNextState() != DeviceState::Exiting) {};
    LOG(WARN) << "1";

    UnsubscribeFromDeviceStateChange();
    LOG(WARN) << "2";
    ReleaseDeviceControl();
    LOG(WARN) << "3";
}

} /* namespace plugins */
} /* namespace mq */
} /* namespace fair */
