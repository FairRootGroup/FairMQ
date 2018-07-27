/********************************************************************************
 *    Copyright (C) 2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_DEVICERUNNER_H
#define FAIR_MQ_DEVICERUNNER_H

#include <fairmq/EventManager.h>
#include <fairmq/PluginManager.h>
#include <FairMQDevice.h>
#include <FairMQLogger.h>
#include <options/FairMQProgOptions.h>

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace fair
{
namespace mq
{

/**
 * @class DeviceRunner DeviceRunner.h <fairmq/DeviceRunner.h>
 * @brief Utility class to facilitate a convenient top-level device launch/shutdown.
 *
 * Runs a single FairMQ device with config and plugin support.
 *
 * For customization user hooks are executed at various steps during device launch/shutdown in the following sequence:
 *
 *         LoadPlugins
 *              |
 *              v
 *   SetCustomCmdLineOptions
 *              |
 *              v
 *    ModifyRawCmdLineArgs
 *              |
 *              v
 *      InstatiateDevice
 *
 * Each hook has access to all members of the DeviceRunner and really only differs by the point in time it is called.
 *
 * For an example usage of this class see the fairmq/runFairMQDevice.h header.
 */
class DeviceRunner
{
  public:
    DeviceRunner(int argc, char* const argv[]);

    auto Run() -> int;
    auto RunWithExceptionHandlers() -> int;

    template<typename H>
    auto AddHook(std::function<void(DeviceRunner&)> hook) -> void { fEvents.Subscribe<H>("runner", hook); }
    template<typename H>
    auto RemoveHook() -> void { fEvents.Unsubscribe<H>("runner"); }

    std::vector<std::string> fRawCmdLineArgs;
    FairMQProgOptions fConfig;
    std::unique_ptr<FairMQDevice> fDevice;
    PluginManager fPluginManager;

  private:
    EventManager fEvents;
};

namespace hooks
{
struct LoadPlugins : Event<DeviceRunner&> {};
struct SetCustomCmdLineOptions : Event<DeviceRunner&> {};
struct ModifyRawCmdLineArgs : Event<DeviceRunner&> {};
struct InstantiateDevice : Event<DeviceRunner&> {};
} /* namespace hooks */

} /* namespace mq */
} /* namespace fair */

#endif /* FAIR_MQ_DEVICERUNNER_H */
