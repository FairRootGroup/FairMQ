/********************************************************************************
 *    Copyright (C) 2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "DDS.h"

#include <termios.h> // for the interactive mode
#include <poll.h> // for the interactive mode
#include <sstream>
#include <iostream>

using namespace std;

namespace fair
{
namespace mq
{
namespace plugins
{

DDS::DDS(const string name, const Plugin::Version version, const string maintainer, const string homepage, PluginServices* pluginServices)
    : Plugin(name, version, maintainer, homepage, pluginServices)
    , fService()
    , fDDSCustomCmd(fService)
    , fDDSKeyValue(fService)
    , fBindingChans()
    , fConnectingChans()
    , fStopMutex()
    , fStopCondition()
    , fCommands({ "INIT DEVICE", "INIT TASK", "PAUSE", "RUN", "STOP", "RESET TASK", "RESET DEVICE" })
    , fControllerThread()
    , fEvents()
    , fEventsMutex()
    , fNewEvent()
    , fDeviceTerminationRequested(false)
{
    try
    {
        TakeDeviceControl();
        fControllerThread = thread(&DDS::HandleControl, this);
    }
    catch (PluginServices::DeviceControlError& e)
    {
        LOG(DEBUG) << e.what();
    }
    catch (exception& e)
    {
        LOG(ERROR) << "Error in plugin initialization: " << e.what();
    }
}

auto DDS::HandleControl() -> void
{
    try
    {
        // subscribe for DDS service errors.
        fService.subscribeOnError([](const dds::intercom_api::EErrorCode errorCode, const string& errorMsg) {
            LOG(ERROR) << "DDS Error received: error code: " << errorCode << ", error message: " << errorMsg << endl;
        });

        // subscribe to device state changes, pushing new state chenges into the event queue
        SubscribeToDeviceStateChange([&](DeviceState newState)
        {
            {
                lock_guard<mutex> lock{fEventsMutex};
                fEvents.push(newState);
            }
            fNewEvent.notify_one();
            if (newState == DeviceState::Exiting)
            {
                fDeviceTerminationRequested = true;
            }
        });

        ChangeDeviceState(DeviceStateTransition::InitDevice);
        while (WaitForNextState() != DeviceState::InitializingDevice) {}

        // in the Initializing state subscribe to receive addresses of connecting channels from DDS
        // and propagate addresses of bound channels to DDS.
        FillChannelContainers();

        if (fConnectingChans.size() > 0)
        {
            LOG(DEBUG) << "Subscribing for DDS properties.";

            SubscribeForConnectingChannels();
        }

        // subscribe for state changes from DDS (subscriptions start firing after fService.start() is called)
        SubscribeForCustomCommands();

        // start DDS service - subscriptions will only start firing after this step
        fService.start();

        // publish bound addresses via DDS at keys corresponding to the channel prefixes, e.g. 'data' in data[i]
        PublishBoundChannels();

        while (WaitForNextState() != DeviceState::DeviceReady) {}

        ChangeDeviceState(DeviceStateTransition::InitTask);
        while (WaitForNextState() != DeviceState::Ready) {}
        ChangeDeviceState(DeviceStateTransition::Run);

        // wait until stop signal
        unique_lock<mutex> lock(fStopMutex);
        while (!fDeviceTerminationRequested)
        {
            fStopCondition.wait_for(lock, chrono::seconds(1));
        }
        LOG(DEBUG) << "Stopping DDS control plugin";
    }
    catch (exception& e)
    {
        LOG(ERROR) << "Error: " << e.what() << endl;
        return;
    }

    fDDSKeyValue.unsubscribe();
    fDDSCustomCmd.unsubscribe();

    UnsubscribeFromDeviceStateChange();
    ReleaseDeviceControl();
}

auto DDS::FillChannelContainers() -> void
{
    unordered_map<string, int> channelInfo(GetChannelInfo());
    for (const auto& c : channelInfo)
    {
        string methodKey{"chans." + c.first + "." + to_string(c.second - 1) + ".method"};
        string addressKey{"chans." + c.first + "." + to_string(c.second - 1) + ".address"};
        if (GetProperty<string>(methodKey) == "bind")
        {
            fBindingChans.insert(make_pair(c.first, vector<string>()));
            for (int i = 0; i < c.second; ++i)
            {
                fBindingChans.at(c.first).push_back(GetProperty<string>(addressKey));
            }
        }
        else if (GetProperty<string>(methodKey) == "connect")
        {
            fConnectingChans.insert(make_pair(c.first, DDSConfig()));
            LOG(DEBUG) << "preparing to connect: " << c.first << " with " << c.second << " sub-channels.";
            for (int i = 0; i < c.second; ++i)
            {
                fConnectingChans.at(c.first).fSubChannelAddresses.push_back(string());
            }
        }
        else
        {
            LOG(ERROR) << "Cannot update address configuration. Channel method (bind/connect) not specified.";
            return;
        }
    }
}

auto DDS::SubscribeForConnectingChannels() -> void
{
    fDDSKeyValue.subscribe([&] (const string& propertyId, const string& key, const string& value)
    {
        LOG(DEBUG) << "Received update for " << propertyId << ": key=" << key << " value=" << value;
        fConnectingChans.at(propertyId).fDDSValues.insert(make_pair<string, string>(key.c_str(), value.c_str()));

        // update channels and remove them from unfinished container
        for (auto mi = fConnectingChans.begin(); mi != fConnectingChans.end(); /* no increment */)
        {
            if (mi->second.fSubChannelAddresses.size() == mi->second.fDDSValues.size())
            {
                // when multiple subChannels are used, their order on every device should be the same, irregardless of arrival order from DDS.
                sort(mi->second.fSubChannelAddresses.begin(), mi->second.fSubChannelAddresses.end());
                auto it = mi->second.fDDSValues.begin();
                for (unsigned int i = 0; i < mi->second.fSubChannelAddresses.size(); ++i)
                {
                    string k = "chans." + mi->first + "." + to_string(i) + ".address";
                    SetProperty<string>(k, it->second);
                    ++it;
                }
                fConnectingChans.erase(mi++);
            }
            else
            {
                ++mi;
            }
        }
    });
}

auto DDS::PublishBoundChannels() -> void
{
    for (const auto& chan : fBindingChans)
    {
        unsigned int index = 0;
        for (const auto& i : chan.second)
        {
            LOG(DEBUG) << "Publishing " << chan.first << "[" << index << "] address to DDS under '" << chan.first << "' property name.";
            fDDSKeyValue.putValue(chan.first, i);
            ++index;
        }
    }
}

auto DDS::SubscribeForCustomCommands() -> void
{
    string id = GetProperty<string>("id");
    string pid(to_string(getpid()));

    fDDSCustomCmd.subscribe([id, pid, this](const string& cmd, const string& cond, uint64_t senderId)
    {
        LOG(INFO) << "Received command: " << cmd;

        if (cmd == "check-state")
        {
            fDDSCustomCmd.send(id + ": " + ToStr(GetCurrentDeviceState()) + " (pid: " + pid + ")", to_string(senderId));
        }
        else if (fCommands.find(cmd) != fCommands.end())
        {
            fDDSCustomCmd.send(id + ": attempting to " + cmd, to_string(senderId));
            ChangeDeviceState(ToDeviceStateTransition(cmd));
        }
        else if (cmd == "END")
        {
            fDDSCustomCmd.send(id + ": attempting to " + cmd, to_string(senderId));
            ChangeDeviceState(ToDeviceStateTransition(cmd));
            fDDSCustomCmd.send(id + ": " + ToStr(GetCurrentDeviceState()), to_string(senderId));
            if (ToStr(GetCurrentDeviceState()) == "EXITING")
            {
                unique_lock<mutex> lock(fStopMutex);
                fStopCondition.notify_one();
            }
        }
        else if (cmd == "dump-config")
        {
            stringstream ss;
            for (const auto pKey: GetPropertyKeys())
            {
                ss << id << ": " << pKey << " -> " << GetPropertyAsString(pKey) << endl;
            }
            fDDSCustomCmd.send(ss.str(), to_string(senderId));
        }
        else
        {
            LOG(WARN) << "Unknown command: " << cmd;
            LOG(WARN) << "Origin: " << senderId;
            LOG(WARN) << "Destination: " << cond;
        }
    });
}

auto DDS::WaitForNextState() -> DeviceState
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

DDS::~DDS()
{
    if (fControllerThread.joinable())
    {
        fControllerThread.join();
    }
}

} /* namespace plugins */
} /* namespace mq */
} /* namespace fair */
