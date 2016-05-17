/* 
 * File:   runSimpleMQStateMachine.h
 * Author: winckler
 *
 * Created on July 2, 2015, 2:07 PM
 */

#ifndef RUNSIMPLEMQSTATEMACHINE_H
#define RUNSIMPLEMQSTATEMACHINE_H

/// std
#include <iostream>
#include <type_traits>
#include <string>

/// boost
#include "boost/program_options.hpp"

/// FairRoot - FairMQ
#include "FairMQLogger.h"
#include "FairMQParser.h"
#include "FairMQProgOptions.h"

// template function that takes any device
// and runs a simple MQ state machine configured from a JSON file
template<typename TMQDevice>
inline int runStateMachine(TMQDevice& device, FairMQProgOptions& config)
{
    device.CatchSignals();

    device.SetConfig(config);
    device.ChangeState(TMQDevice::INIT_DEVICE);
    device.WaitForEndOfState(TMQDevice::INIT_DEVICE);

    device.ChangeState(TMQDevice::INIT_TASK);
    device.WaitForEndOfState(TMQDevice::INIT_TASK);

    device.ChangeState(TMQDevice::RUN);

    std::string control = config.GetValue<std::string>("control");

    // TODO: Extend this with DDS (requires optional dependency?)?
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
        LOG(ERROR) << "Requested control mechanism '" << control << "' is not available.";
        LOG(ERROR) << "Currently available are:"
                   << " 'interactive'"
                   << ", 'static'"
                   << ". Exiting.";
        exit(EXIT_FAILURE);
    }

    return 0;
}

#endif /* RUNSIMPLEMQSTATEMACHINE_H */
