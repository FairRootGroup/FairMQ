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

#include <iostream>
#include <string>
#include <chrono>
#include <dlfcn.h>

// template function that takes any device
// and runs a simple MQ state machine configured from a JSON file and/or a plugin.
template<typename TMQDevice>
inline int runStateMachine(TMQDevice& device, FairMQProgOptions& cfg)
{
    device.RegisterChannelEndpoints();
    if (cfg.Count("print-channels"))
    {
        device.PrintRegisteredChannels();
        device.ChangeState(TMQDevice::END);
        return 0;
    }

    if (cfg.GetValue<int>("catch-signals") > 0)
    {
        device.CatchSignals();
    }
    else
    {
        LOG(WARN) << "Signal handling (e.g. ctrl+C) has been deactivated via command line argument";
    }

    LOG(DEBUG) << "PID: " << getpid();

    device.SetConfig(cfg);
    std::string config = cfg.GetValue<std::string>("config");
    std::string control = cfg.GetValue<std::string>("control");

    std::clock_t cStart = std::clock();
    auto tStart = std::chrono::high_resolution_clock::now();

    device.ChangeState(TMQDevice::INIT_DEVICE);
    // Wait for the binding channels to bind
    device.WaitForInitialValidation();

    device.WaitForEndOfState(TMQDevice::INIT_DEVICE);

    std::clock_t cEnd = std::clock();
    auto tEnd = std::chrono::high_resolution_clock::now();

    LOG(DEBUG) << "Init time (CPU) : " << std::fixed << std::setprecision(2) << 1000.0 * (cEnd - cStart) / CLOCKS_PER_SEC << " ms";
    LOG(DEBUG) << "Init time (Wall): " << std::chrono::duration<double, std::milli>(tEnd - tStart).count() << " ms";

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
