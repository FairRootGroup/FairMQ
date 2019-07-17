/********************************************************************************
 *    Copyright (C) 2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <fairmq/PluginServices.h>

using namespace fair::mq;
using namespace std;

auto PluginServices::ChangeDeviceState(const string& controller, const DeviceStateTransition next) -> bool
{
    lock_guard<mutex> lock{fDeviceControllerMutex};

    if (!fDeviceController) fDeviceController = controller;

    if (fDeviceController == controller) {
        return fDevice.ChangeState(next);
    } else {
        throw DeviceControlError{tools::ToString(
            "Plugin '", controller, "' is not allowed to change device states. ",
            "Currently, plugin '", *fDeviceController, "' has taken control."
        )};
    }
}

void PluginServices::TransitionDeviceStateTo(const std::string& controller, DeviceState state)
{
    lock_guard<mutex> lock{fDeviceControllerMutex};

    if (!fDeviceController) fDeviceController = controller;

    if (fDeviceController == controller) {
        fDevice.TransitionTo(state);
    } else {
        throw DeviceControlError{tools::ToString(
            "Plugin '", controller, "' is not allowed to change device states. ",
            "Currently, plugin '", *fDeviceController, "' has taken control."
        )};
    }
}

auto PluginServices::TakeDeviceControl(const string& controller) -> void
{
    lock_guard<mutex> lock{fDeviceControllerMutex};

    if (!fDeviceController) {
        fDeviceController = controller;
    } else if (fDeviceController == controller) {
        // nothing to do
    } else {
        throw DeviceControlError{tools::ToString(
            "Plugin '", controller, "' is not allowed to take over control. ",
            "Currently, plugin '", *fDeviceController, "' has taken control."
        )};
    }
}

auto PluginServices::StealDeviceControl(const string& controller) -> void
{
    lock_guard<mutex> lock{fDeviceControllerMutex};

    fDeviceController = controller;
}

auto PluginServices::ReleaseDeviceControl(const string& controller) -> void
{
    {
        lock_guard<mutex> lock{fDeviceControllerMutex};

        if (fDeviceController == controller) {
            fDeviceController = boost::none;
        } else {
            throw DeviceControlError{tools::ToString("Plugin '", controller, "' cannot release control because it has not taken over control.")};
        }
    }

    fReleaseDeviceControlCondition.notify_one();
}

auto PluginServices::GetDeviceController() const -> boost::optional<string>
{
    lock_guard<mutex> lock{fDeviceControllerMutex};

    return fDeviceController;
}

auto PluginServices::WaitForReleaseDeviceControl() -> void
{
    unique_lock<mutex> lock{fDeviceControllerMutex};

    while (fDeviceController) {
        fReleaseDeviceControlCondition.wait(lock);
    }
}
