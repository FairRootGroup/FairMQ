/********************************************************************************
 *    Copyright (C) 2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_PLUGINS_CONTROL
#define FAIR_MQ_PLUGINS_CONTROL

#include <fairmq/Plugin.h>

#include <condition_variable>
#include <mutex>
#include <string>
#include <queue>
#include <thread>
#include <atomic>

namespace fair
{
namespace mq
{
namespace plugins
{

class Control : public Plugin
{
  public:
    Control(const std::string name, const Plugin::Version version, const std::string maintainer, const std::string homepage, PluginServices* pluginServices);

    ~Control();

  private:
    auto InteractiveMode() -> void;
    auto PrintInteractiveHelp() -> void;
    auto StaticMode() -> void;
    auto WaitForNextState() -> DeviceState;
    auto SignalHandler(int signal) -> void;
    auto RunShutdownSequence() -> void;
    auto RunStartupSequence() -> void;
    auto EmptyEventQueue() -> void;

    std::thread fControllerThread;
    std::thread fSignalHandlerThread;
    std::queue<DeviceState> fEvents;
    std::mutex fEventsMutex;
    std::condition_variable fNewEvent;
    std::atomic<bool> fDeviceTerminationRequested;
}; /* class Control */

auto ControlPluginProgramOptions() -> Plugin::ProgOptions;

REGISTER_FAIRMQ_PLUGIN(
    Control,                                     // Class name
    control,                                     // Plugin name (string, lower case chars only)
    (Plugin::Version{1,0,0}),                    // Version
    "FairRootGroup <fairroot@gsi.de>",           // Maintainer
    "https://github.com/FairRootGroup/FairRoot", // Homepage
    ControlPluginProgramOptions                  // Free function which declares custom program options for the plugin
                                                 // signature: () -> boost::optional<boost::program_options::options_description>
)

} /* namespace plugins */
} /* namespace mq */
} /* namespace fair */

#endif /* FAIR_MQ_PLUGINS_CONTROL */
