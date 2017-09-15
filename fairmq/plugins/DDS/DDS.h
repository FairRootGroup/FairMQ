/********************************************************************************
 *    Copyright (C) 2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_PLUGINS_DDS
#define FAIR_MQ_PLUGINS_DDS

#include <fairmq/Plugin.h>

#include <dds_intercom.h>

#include <condition_variable>
#include <mutex>
#include <string>
#include <queue>
#include <thread>
#include <vector>
#include <unordered_map>
#include <set>

namespace fair
{
namespace mq
{
namespace plugins
{

struct DDSConfig
{
    DDSConfig()
        : fSubChannelAddresses()
        , fDDSValues()
    {}

    // container of sub channel addresses
    std::vector<std::string> fSubChannelAddresses;
    // dds values for the channel
    std::unordered_map<std::string, std::string> fDDSValues;
};

class DDS : public Plugin
{
  public:
    DDS(const std::string name, const Plugin::Version version, const std::string maintainer, const std::string homepage, PluginServices* pluginServices);

    ~DDS();

  private:
    auto HandleControl() -> void;
    auto WaitForNextState() -> DeviceState;

    auto FillChannelContainers() -> void;
    auto SubscribeForConnectingChannels() -> void;
    auto PublishBoundChannels() -> void;
    auto SubscribeForStateChanges() -> void;

    dds::intercom_api::CIntercomService fService;
    dds::intercom_api::CCustomCmd fDDSCustomCmd;
    dds::intercom_api::CKeyValue fDDSKeyValue;

    std::unordered_map<std::string, std::vector<std::string>> fBindingChans;
    std::unordered_map<std::string, DDSConfig> fConnectingChans;

    std::mutex fStopMutex;
    std::condition_variable fStopCondition;

    const std::set<std::string> fCommands;

    std::thread fControllerThread;
    std::queue<DeviceState> fEvents;
    std::mutex fEventsMutex;
    std::condition_variable fNewEvent;
};

REGISTER_FAIRMQ_PLUGIN(
    DDS,                                         // Class name
    dds,                                         // Plugin name (string, lower case chars only)
    (Plugin::Version{1,0,0}),                    // Version
    "FairRootGroup <fairroot@gsi.de>",           // Maintainer
    "https://github.com/FairRootGroup/FairRoot", // Homepage
    fair::mq::Plugin::NoProgramOptions           // custom program options for the plugin
)

} /* namespace plugins */
} /* namespace mq */
} /* namespace fair */

#endif /* FAIR_MQ_PLUGINS_DDS */
