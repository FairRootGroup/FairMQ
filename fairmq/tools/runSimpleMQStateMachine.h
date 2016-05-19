/* 
 * File:   runSimpleMQStateMachine.h
 * Author: winckler
 *
 * Created on July 2, 2015, 2:07 PM
 */

#ifndef RUNSIMPLEMQSTATEMACHINE_H
#define RUNSIMPLEMQSTATEMACHINE_H

#include <iostream>
#include <string>

#ifdef DDS_FOUND
#include "FairMQDDSTools.h"
#endif /*DDS_FOUND*/

#include "FairMQLogger.h"
#include "FairMQParser.h"
#include "FairMQProgOptions.h"

// template function that takes any device
// and runs a simple MQ state machine configured from a JSON file and/or (optionally) DDS.
template<typename TMQDevice>
inline int runStateMachine(TMQDevice& device, FairMQProgOptions& config)
{
    device.CatchSignals();

    device.SetConfig(config);
    std::string control = config.GetValue<std::string>("control");

    device.ChangeState(TMQDevice::INIT_DEVICE);
#ifdef DDS_FOUND
    if (control == "dds")
    {
        HandleConfigViaDDS(device);
    }
#endif /*DDS_FOUND*/
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
#ifdef DDS_FOUND
    else if (control == "dds")
    {
        runDDSStateHandler(device);
    }
#endif /*DDS_FOUND*/
    else
    {
        LOG(ERROR) << "Requested control mechanism '" << control << "' is not available.";
        LOG(ERROR) << "Currently available are:"
                   << " 'interactive'"
                   << ", 'static'"
#ifdef DDS_FOUND
                   << ", 'dds'"
#endif /*DDS_FOUND*/
                   << ". Exiting.";
        exit(EXIT_FAILURE);
    }

    return 0;
}

#endif /* RUNSIMPLEMQSTATEMACHINE_H */
