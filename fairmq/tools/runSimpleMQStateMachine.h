/* 
 * File:   runSimpleMQStateMachine.h
 * Author: winckler
 *
 * Created on July 2, 2015, 2:07 PM
 */

#ifndef RUNSIMPLEMQSTATEMACHINE_H
#define RUNSIMPLEMQSTATEMACHINE_H

#include "FairMQLogger.h"
#include "options/FairMQParser.h"
#include "options/FairMQProgOptions.h"
#include <FairMQDevice.h>
#include <fairmq/PluginManager.h>

#include <iostream>
#include <string>
#include <chrono>
#include <dlfcn.h>

// template function that takes any device
// and runs a simple MQ state machine configured from a JSON file and/or a plugin.
template<typename TMQDevice>
inline int runStateMachine(TMQDevice& device, FairMQProgOptions& cfg)
{
    std::string config = cfg.GetValue<std::string>("config");
    std::string control = cfg.GetValue<std::string>("control");

    device.ChangeState(TMQDevice::INIT_DEVICE);
    // Wait for the binding channels to bind
    device.WaitForInitialValidation();

    device.WaitForEndOfState(TMQDevice::INIT_DEVICE);

    device.ChangeState(TMQDevice::INIT_TASK);
    device.WaitForEndOfState(TMQDevice::INIT_TASK);

    device.ChangeState(TMQDevice::RUN);

    if (control == "interactive")
    {
        device.InteractiveStateLoop();
    }
    else if (control == "static")
    {
        device.WaitForEndOfState(TMQDevice::RUN);

        if (!device.CheckCurrentState(TMQDevice::EXITING))
        {
            device.ChangeState(TMQDevice::RESET_TASK);
            device.WaitForEndOfState(TMQDevice::RESET_TASK);

            device.ChangeState(TMQDevice::RESET_DEVICE);
            device.WaitForEndOfState(TMQDevice::RESET_DEVICE);

            device.ChangeState(TMQDevice::END);
        }
    }
    else
    {
    }

    return 0;
}

#endif /* RUNSIMPLEMQSTATEMACHINE_H */
