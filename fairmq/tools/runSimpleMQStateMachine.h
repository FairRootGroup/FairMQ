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

/// ZMQ/nmsg (in FairSoft)
#ifdef NANOMSG
#include "FairMQTransportFactoryNN.h"
#else
#include "FairMQTransportFactoryZMQ.h"
#endif

/// FairRoot - FairMQ
#include "FairMQLogger.h"
#include "FairMQParser.h"
#include "FairMQProgOptions.h"



// template function that take any device, 
// and run a simple MQ state machine configured from a JSON file
template<typename TMQDevice>
inline int runStateMachine(TMQDevice& device, FairMQProgOptions& config)
{
    device.CatchSignals();
    std::string jsonfile = config.GetValue<std::string>("config-json-file");
    std::string id = config.GetValue<std::string>("id");
    int ioThreads = config.GetValue<int>("io-threads");

    config.UserParser<FairMQParser::JSON>(jsonfile, id);

    device.fChannels = config.GetFairMQMap();

    device.SetProperty(TMQDevice::Id, id);
    device.SetProperty(TMQDevice::NumIoThreads, ioThreads);

    LOG(INFO) << "PID: " << getpid();

#ifdef NANOMSG
    FairMQTransportFactory* transportFactory = new FairMQTransportFactoryNN();
#else
    FairMQTransportFactory* transportFactory = new FairMQTransportFactoryZMQ();
#endif

    device.SetTransport(transportFactory);

    device.ChangeState(TMQDevice::INIT_DEVICE);
    device.WaitForEndOfState(TMQDevice::INIT_DEVICE);

    device.ChangeState(TMQDevice::INIT_TASK);
    device.WaitForEndOfState(TMQDevice::INIT_TASK);

    device.ChangeState(TMQDevice::RUN);
    device.InteractiveStateLoop();

    return 0;
}

template<typename TMQDevice>
inline int runNonInteractiveStateMachine(TMQDevice& device, FairMQProgOptions& config)
{
    device.CatchSignals();
    std::string jsonfile = config.GetValue<std::string>("config-json-file");
    std::string id = config.GetValue<std::string>("id");
    int ioThreads = config.GetValue<int>("io-threads");

    config.UserParser<FairMQParser::JSON>(jsonfile, id);

    device.fChannels = config.GetFairMQMap();

    device.SetProperty(TMQDevice::Id, id);
    device.SetProperty(TMQDevice::NumIoThreads, ioThreads);

    LOG(INFO) << "PID: " << getpid();

#ifdef NANOMSG
    FairMQTransportFactory* transportFactory = new FairMQTransportFactoryNN();
#else
    FairMQTransportFactory* transportFactory = new FairMQTransportFactoryZMQ();
#endif

    device.SetTransport(transportFactory);

    device.ChangeState(TMQDevice::INIT_DEVICE);
    device.WaitForEndOfState(TMQDevice::INIT_DEVICE);

    device.ChangeState(TMQDevice::INIT_TASK);
    device.WaitForEndOfState(TMQDevice::INIT_TASK);

    device.ChangeState(TMQDevice::RUN);
    device.WaitForEndOfState(TMQDevice::RUN);

    device.ChangeState(TMQDevice::RESET_TASK);
    device.WaitForEndOfState(TMQDevice::RESET_TASK);

    device.ChangeState(TMQDevice::RESET_DEVICE);
    device.WaitForEndOfState(TMQDevice::RESET_DEVICE);

    device.ChangeState(TMQDevice::END);

    return 0;
}


#endif /* RUNSIMPLEMQSTATEMACHINE_H */

