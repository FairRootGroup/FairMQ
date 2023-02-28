/********************************************************************************
 *    Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_TEST_CONTROL_H
#define FAIR_MQ_TEST_CONTROL_H

#include <fairmq/Device.h>

namespace fair::mq::test
{

enum class Cycle {
    FullNoStop,
    ToDeviceReadyAndBack,
    ToRun,
    ReadyToEnd
};

inline auto Control(Device& device, Cycle cycle = Cycle::FullNoStop)
{
    if(   cycle == Cycle::FullNoStop
       || cycle == Cycle::ToDeviceReadyAndBack
       || cycle == Cycle::ToRun)
    {
        device.ChangeStateOrThrow(Transition::InitDevice);
        device.WaitForState(State::InitializingDevice);
        device.ChangeStateOrThrow(Transition::CompleteInit);
        device.WaitForState(State::Initialized);
        device.ChangeStateOrThrow(Transition::Bind);
        device.WaitForState(State::Bound);
        device.ChangeStateOrThrow(Transition::Connect);
        device.WaitForState(State::DeviceReady);
    }
    if(   cycle == Cycle::FullNoStop
       || cycle == Cycle::ToRun)
    {
        device.ChangeStateOrThrow(Transition::InitTask);
        device.WaitForState(State::Ready);
        device.ChangeStateOrThrow(Transition::Run);
        device.WaitForState(State::Running);
    }
    if(   cycle == Cycle::FullNoStop
       || cycle == Cycle::ReadyToEnd)
    {
        device.WaitForState(State::Ready);
        device.ChangeStateOrThrow(Transition::ResetTask);
        device.WaitForState(State::DeviceReady);
    }
    if(   cycle == Cycle::FullNoStop
       || cycle == Cycle::ToDeviceReadyAndBack
       || cycle == Cycle::ReadyToEnd)
    {
        device.ChangeStateOrThrow(Transition::ResetDevice);
        device.WaitForState(State::Idle);
        device.ChangeStateOrThrow(Transition::End);
    }
}

}

#endif   // FAIR_MQ_TEST_CONTROL_H
