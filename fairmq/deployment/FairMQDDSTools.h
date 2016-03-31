#ifndef FAIRMQDDSTOOLS_H_
#define FAIRMQDDSTOOLS_H_

#include "FairMQLogger.h"
#include "FairMQDevice.h"
#include "FairMQChannel.h"

#include <vector>
#include <map>
#include <string>
#include <exception>
#include <condition_variable>
#include <mutex>
#include <cstdlib>

#include "dds_intercom.h" // DDS

using namespace std;
using namespace dds::intercom_api;

// container to hold channel config and corresponding dds key values
struct DDSConfig
{
    // container of sub channels, e.g. 'i' in data[i]
    vector<FairMQChannel*> subChannels;
    // dds values for the channel
    CKeyValue::valuesMap_t ddsValues;
};

/// Handles channels addresses of the device with configuration from DDS
/// Addresses of binding channels are published via DDS using channels names as keys
/// Addresses of connecting channels are collected from DDS using channels names as keys
/// \param device  Reference to FairMQDevice whose channels to handle
void HandleConfigViaDDS(FairMQDevice& device)
{
    // container for binding channels
    vector<FairMQChannel*> bindingChans;
    // container for connecting channels
    map<string, DDSConfig> connectingChans;

    // fill the containers
    for (auto& mi : device.fChannels)
    {
        if ((mi.second).at(0).GetMethod() == "bind")
        {
            for (auto& vi : mi.second)
            {
                bindingChans.push_back(&vi);
            }
        }
        else if ((mi.second).at(0).GetMethod() == "connect")
        {
            // try some trickery with forwarding emplacing values into map
            connectingChans.emplace(piecewise_construct, forward_as_tuple(mi.first), forward_as_tuple());
            for (auto& vi : mi.second)
            {
                connectingChans.at(mi.first).subChannels.push_back(&vi);
            }
        }
        else
        {
            LOG(ERROR) << "Cannot update address configuration. Socket method (bind/connect) not specified.";
            return;
        }
    }

    // Wait for the binding channels to bind
    device.WaitForInitialValidation();

    // DDS key value store
    CKeyValue ddsKV;

    // publish bound addresses via DDS at keys corresponding to the channel prefixes, e.g. 'data' in data[i]
    for (const auto& i : bindingChans)
    {
        // LOG(INFO) << "Publishing " << i->GetChannelPrefix() << " address to DDS under '" << i->GetProperty() << "' property name.";
        ddsKV.putValue(i->GetProperty(), i->GetAddress());
    }

    // receive connect addresses from DDS via keys corresponding to channel prefixes, e.g. 'data' in data[i]
    if (connectingChans.size() > 0)
    {
        mutex keyMutex;
        condition_variable keyCV;

        LOG(DEBUG) << "Subscribing for DDS properties.";
        ddsKV.subscribe([&] (const string& /*key*/, const string& /*value*/)
        {
            keyCV.notify_all();
        });

        // scope based locking
        {
            unique_lock<mutex> lock(keyMutex);
            keyCV.wait_for(lock, chrono::milliseconds(1000), [&] ()
            {
                // receive new properties
                for (auto& mi : connectingChans)
                {
                    for (auto& vi : mi.second.subChannels)
                    {
                        // LOG(INFO) << "Waiting for " << vi->GetChannelPrefix() << " address from DDS.";
                        ddsKV.getValues(vi->GetProperty(), &(mi.second.ddsValues));
                    }
                }

                // update channels and remove them from unfinished container
                for (auto mi = connectingChans.begin(); mi != connectingChans.end(); /* no increment */)
                {
                    if (mi->second.subChannels.size() == mi->second.ddsValues.size())
                    {
                        auto it = mi->second.ddsValues.begin();
                        for (int i = 0; i < mi->second.subChannels.size(); ++i)
                        {
                          mi->second.subChannels.at(i)->UpdateAddress(it->second);
                          ++it;
                        }
                        // when multiple subChannels are used, their order on every device should be the same, irregardless of arrival order from DDS.
                        device.SortChannel(mi->first);
                        connectingChans.erase(mi++);
                    }
                    else
                    {
                        ++mi;
                    }
                }

                if (connectingChans.empty())
                {
                    LOG(DEBUG) << "Successfully received all required DDS properties!";
                }
                return connectingChans.empty();
            });
        }
    }
}

/// Controls device state via DDS custom commands interface
/// \param device  Reference to FairMQDevice whose state to control
void runDDSStateHandler(FairMQDevice& device)
{
    mutex mtx;
    condition_variable stopCondition;

    string id = device.GetProperty(FairMQDevice::Id, "");
    string pid(to_string(getpid()));

    try
    {
        const set<string> events = { "INIT_DEVICE", "INIT_TASK", "PAUSE", "RUN", "STOP", "RESET_TASK", "RESET_DEVICE" };

        CCustomCmd ddsCustomCmd;

        ddsCustomCmd.subscribe([&](const string& cmd, const string& cond, uint64_t senderId)
        {
            LOG(INFO) << "Received command: " << cmd;

            if (cmd == "check-state")
            {
                ddsCustomCmd.send(id + ": " + device.GetCurrentStateName() + " (pid: " + pid + ")", to_string(senderId));
            }
            else if (events.find(cmd) != events.end())
            {
                ddsCustomCmd.send(id + ": attempting to " + cmd, to_string(senderId));
                device.ChangeState(cmd);
            }
            else if (cmd == "END")
            {
                ddsCustomCmd.send(id + ": attempting to " + cmd, to_string(senderId));
                device.ChangeState(cmd);
                ddsCustomCmd.send(id + ": " + device.GetCurrentStateName(), to_string(senderId));
                if (device.GetCurrentStateName() == "EXITING")
                {
                    unique_lock<mutex> lock(mtx);
                    stopCondition.notify_one();
                }
            }
            else
            {
                LOG(WARN) << "Unknown command: " << cmd;
                LOG(WARN) << "Origin: " << senderId;
                LOG(WARN) << "Destination: " << cond;
            }
        });

        LOG(INFO) << "Listening for commands from DDS...";
        unique_lock<mutex> lock(mtx);
        stopCondition.wait(lock);
    }
    catch (exception& e)
    {
        cerr << "Error: " << e.what() << endl;
        return;
    }
}

#endif /* FAIRMQDDSTOOLS_H_ */
