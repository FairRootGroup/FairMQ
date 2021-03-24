/********************************************************************************
 * Copyright (C) 2017-2021 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_DEVICERUNNER_H
#define FAIR_MQ_DEVICERUNNER_H

#include <fairmq/Device.h>
#include <fairmq/EventManager.h>
#include <fairmq/PluginManager.h>
#include <fairmq/ProgOptions.h>

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace fair::mq
{

/**
 * @class DeviceRunner DeviceRunner.h <fairmq/DeviceRunner.h>
 * @brief Utility class to facilitate a convenient top-level device launch/shutdown.
 *
 * Runs a single FairMQ device with config and plugin support.
 *
 * For customization user hooks are executed at various steps during device launch/shutdown in the
 * following sequence:
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
 * Each hook has access to all members of the DeviceRunner and really only differs by the point in
 * time it is called.
 *
 * For an example usage of this class see the fairmq/runFairMQDevice.h header.
 */
class DeviceRunner
{
  public:
    DeviceRunner(int argc, char*const* argv, bool printLogo = true);

    auto Run() -> int;
    auto RunWithExceptionHandlers() -> int;

    static bool HandleGeneralOptions(const fair::mq::ProgOptions& config, bool printLogo = true);

    void SubscribeForConfigChange();
    void UnsubscribeFromConfigChange();

    template<typename H>
    auto AddHook(std::function<void(DeviceRunner&)> hook) -> void
    {
        fEvents.Subscribe<H>("runner", hook);
    }
    template<typename H>
    auto RemoveHook() -> void
    {
        fEvents.Unsubscribe<H>("runner");
    }

    std::vector<std::string> fRawCmdLineArgs;
    fair::mq::ProgOptions fConfig;
    std::unique_ptr<Device> fDevice;
    PluginManager fPluginManager;
    const bool fPrintLogo;

  private:
    EventManager fEvents;
};

namespace hooks {
struct LoadPlugins : Event<DeviceRunner&> {};
struct SetCustomCmdLineOptions : Event<DeviceRunner&> {};
struct ModifyRawCmdLineArgs : Event<DeviceRunner&> {};
struct InstantiateDevice : Event<DeviceRunner&> {};
} /* namespace hooks */

} // namespace fair::mq

#endif /* FAIR_MQ_DEVICERUNNER_H */
