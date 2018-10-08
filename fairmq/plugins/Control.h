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
#include <fairmq/Version.h>

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
    Control(const std::string& name, const Plugin::Version version, const std::string& maintainer, const std::string& homepage, PluginServices* pluginServices);

    ~Control();

  private:
    auto InteractiveMode() -> void;
    auto PrintInteractiveHelp() -> void;
    auto StaticMode() -> void;
    auto WaitForNextState() -> DeviceState;
    auto SignalHandler() -> void;
    auto HandleShutdownSignal() -> void;
    auto RunShutdownSequence() -> void;
    auto RunStartupSequence() -> void;
    auto EmptyEventQueue() -> void;

    std::thread fControllerThread;
    std::thread fSignalHandlerThread;
    std::thread fShutdownThread;
    std::queue<DeviceState> fEvents;
    std::mutex fEventsMutex;
    std::mutex fShutdownMutex;
    std::condition_variable fNewEvent;
    std::atomic<bool> fDeviceTerminationRequested;
    std::atomic<bool> fHasShutdown;
}; /* class Control */

auto ControlPluginProgramOptions() -> Plugin::ProgOptions;

REGISTER_FAIRMQ_PLUGIN(
    Control,   // Class name
    control,   // Plugin name (string, lower case chars only)
    (Plugin::Version{FAIRMQ_VERSION_MAJOR,
                     FAIRMQ_VERSION_MINOR,
                     FAIRMQ_VERSION_PATCH}),       // Version
    "FairRootGroup <fairroot@gsi.de>",             // Maintainer
    "https://github.com/FairRootGroup/FairRoot",   // Homepage
    ControlPluginProgramOptions   // Free function which declares custom program options for the
                                  // plugin signature: () ->
                                  // boost::optional<boost::program_options::options_description>
)

} /* namespace plugins */
} /* namespace mq */
} /* namespace fair */

#endif /* FAIR_MQ_PLUGINS_CONTROL */
