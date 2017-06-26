/********************************************************************************
 *    Copyright (C) 2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_PLUGINSERVICES_H
#define FAIR_MQ_PLUGINSERVICES_H

#include <FairMQDevice.h>
#include <options/FairMQProgOptions.h>
#include <functional>
#include <string>

namespace fair
{
namespace mq
{

/**
 * @class PluginServices PluginServices.h <fairmq/PluginServices.h>
 * @brief Facilitates communication between devices and plugins
 *
 * - Configuration interface
 * - Control interface
 */
class PluginServices
{
    public:

    PluginServices() = delete;
    PluginServices(FairMQProgOptions& config, FairMQDevice& device)
    : fDevice{device}
    , fConfig{config}
    {}

    // Control
    //enum class DeviceState
    //{
        //Error,
        //Idle,
        //Initializing_device,
        //Device_ready,
        //Initializing_task,
        //Ready,
        //Running,
        //Paused,
        //Resetting_task,
        //Resetting_device,
        //Exiting
    //}

        //auto ToDeviceState(std::string state) const -> DeviceState;

        //auto ChangeDeviceState(DeviceState next) -> void;

        //auto SubscribeToDeviceStateChange(std::function<void(DeviceState [>new<])> callback) -> void;
        //auto UnsubscribeFromDeviceChange() -> void;

        //// Configuration

        //// Writing only works during Initializing_device state
        //template<typename T>
        //auto SetProperty(const std::string& key, T val) -> void;

        //template<typename T>
        //auto GetProperty(const std::string& key) const -> T;
        //auto GetPropertyAsString(const std::string& key) const -> std::string;

        //template<typename T>
        //auto SubscribeToPropertyChange(
            //const std::string& key,
            //std::function<void(const std::string& [>key*/, const T /*newValue<])> callback
        //) const -> void;
        //auto UnsubscribeFromPropertyChange(const std::string& key) -> void;

    private:

    FairMQDevice& fDevice;
    FairMQProgOptions& fConfig;
}; /* class PluginServices */

} /* namespace mq */
} /* namespace fair */

#endif /* FAIR_MQ_PLUGINSERVICES_H */
