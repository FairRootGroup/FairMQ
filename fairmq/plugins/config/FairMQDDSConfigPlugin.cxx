/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "dds_intercom.h"

#include "FairMQConfigPlugin.h"

#include "FairMQLogger.h"
#include "FairMQDevice.h"
#include "FairMQChannel.h"

#include <vector>
#include <map>
#include <string>
#include <exception>
#include <unordered_map>

using namespace std;
using namespace dds::intercom_api;

// container to hold channel config and corresponding dds key values
struct DDSConfig
{
    DDSConfig()
        : subChannels()
        , ddsValues()
    {}

    // container of sub channels, e.g. 'i' in data[i]
    vector<FairMQChannel*> subChannels;
    // dds values for the channel
    unordered_map<string, string> ddsValues;
};

class FairMQConfigPluginDDS
{
  public:
    static FairMQConfigPluginDDS* GetInstance()
    {
        if (fInstance == NULL)
        {
            fInstance  = new FairMQConfigPluginDDS();
        }
        return fInstance;
    }

   static void ResetInstance()
   {
        try
        {
            delete fInstance;
            fInstance = NULL;
        }
        catch (exception& e)
        {
            LOG(ERROR) << "Error: " << e.what() << endl;
            return;
        }
   }

    void Init(FairMQDevice& device)
    {
        for (auto& mi : device.fChannels)
        {
            if ((mi.second).at(0).GetMethod() == "bind")
            {
                for (auto& vi : mi.second)
                {
                    fBindingChans.push_back(&vi);
                }
            }
            else if ((mi.second).at(0).GetMethod() == "connect")
            {
                // try some trickery with forwarding emplacing values into map
                fConnectingChans.emplace(piecewise_construct, forward_as_tuple(mi.first), forward_as_tuple());
                LOG(DEBUG) << "preparing to connect: " << (mi.second).at(0).GetChannelPrefix() << " with " << mi.second.size() << " sockets.";
                for (auto& vi : mi.second)
                {
                    fConnectingChans.at(mi.first).subChannels.push_back(&vi);
                }
            }
            else
            {
                LOG(ERROR) << "Cannot update address configuration. Socket method (bind/connect) not specified.";
                return;
            }
        }

        if (fConnectingChans.size() > 0)
        {
            LOG(DEBUG) << "Subscribing for DDS properties.";

            fDDSKeyValue.subscribe([&] (const string& propertyId, const string& key, const string& value)
            {
                LOG(DEBUG) << "Received update for " << propertyId << ": key=" << key << " value=" << value;
                fConnectingChans.at(propertyId).ddsValues.insert(make_pair<string, string>(key.c_str(), value.c_str()));

                // update channels and remove them from unfinished container
                for (auto mi = fConnectingChans.begin(); mi != fConnectingChans.end(); /* no increment */)
                {
                    if (mi->second.subChannels.size() == mi->second.ddsValues.size())
                    {
                        auto it = mi->second.ddsValues.begin();
                        for (unsigned int i = 0; i < mi->second.subChannels.size(); ++i)
                        {
                          mi->second.subChannels.at(i)->UpdateAddress(it->second);
                          ++it;
                        }
                        // when multiple subChannels are used, their order on every device should be the same, irregardless of arrival order from DDS.
                        device.SortChannel(mi->first);
                        fConnectingChans.erase(mi++);
                    }
                    else
                    {
                        ++mi;
                    }
                }
            });
        }
    }

    void Run(FairMQDevice& device)
    {
        // start DDS intercom service
        fService.start();

        // publish bound addresses via DDS at keys corresponding to the channel prefixes, e.g. 'data' in data[i]
        for (const auto& i : fBindingChans)
        {
            LOG(DEBUG) << "Publishing " << i->GetChannelPrefix() << " address to DDS under '" << i->GetChannelPrefix() << "' property name.";
            fDDSKeyValue.putValue(i->GetChannelPrefix(), i->GetAddress());
        }
    }

  private:
    FairMQConfigPluginDDS()
        : fService()
        , fDDSKeyValue(fService)
        , fBindingChans()
        , fConnectingChans()
    {
        fService.subscribeOnError([](EErrorCode errorCode, const string& msg) {
            LOG(ERROR) << "DDS key-value error code: " << errorCode << ", message: " << msg;
        });
    }

    static FairMQConfigPluginDDS* fInstance;

    CIntercomService fService;
    CKeyValue fDDSKeyValue;

    // container for binding channels
    vector<FairMQChannel*> fBindingChans;
    // container for connecting channels
    map<string, DDSConfig> fConnectingChans;
};

FairMQConfigPluginDDS* FairMQConfigPluginDDS::fInstance = NULL;

void initConfig(FairMQDevice& device)
{
    FairMQConfigPluginDDS::GetInstance()->Init(device);
}

/// Handles channels addresses of the device with configuration from DDS
/// Addresses of binding channels are published via DDS using channels names as keys
/// Addresses of connecting channels are collected from DDS using channels names as keys
/// \param device  Reference to FairMQDevice whose channels to handle
void handleInitialConfig(FairMQDevice& device)
{
    FairMQConfigPluginDDS::GetInstance()->Run(device);
}

void stopConfig()
{
    LOG(DEBUG) << "[FairMQConfigPluginDDS]: " << "Resetting instance.";
    FairMQConfigPluginDDS::ResetInstance();
    LOG(DEBUG) << "[FairMQConfigPluginDDS]: " << "Instance has been reset.";
}

FairMQConfigPlugin fairmqConfigPlugin = { initConfig, handleInitialConfig, stopConfig };
