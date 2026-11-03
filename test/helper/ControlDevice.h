/********************************************************************************
 *    Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_TEST_CONTROL_H
#define FAIR_MQ_TEST_CONTROL_H

#include "Sync.h"

#include <fairmq/Device.h>

#include <thread>

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


/// Drive the device to Exiting from any state. Use it in cleanup code: a test
/// that fails in the middle of a cycle must not leave the state machine thread
/// waiting forever.
inline auto Shutdown(Device& device)
{
    auto changeAndWait = [&](Transition transition, State from) {
        if (device.ChangeState(transition)) {
            WaitUntil([&] { return device.GetCurrentState() != from; });
        }
    };

    while (true) {
        auto const state = device.GetCurrentState();
        switch (state) {
            case State::Exiting:
            case State::Error:
                return;
            case State::Idle:
                static_cast<void>(device.ChangeState(Transition::End));
                return;
            case State::InitializingDevice:
                changeAndWait(Transition::CompleteInit, state);
                break;
            case State::Initialized:
            case State::Bound:
            case State::DeviceReady:
                changeAndWait(Transition::ResetDevice, state);
                break;
            case State::Ready:
                changeAndWait(Transition::ResetTask, state);
                break;
            case State::Running:
                changeAndWait(Transition::Stop, state);
                break;
            default: // a transitional state, wait for the next one
                if (!WaitUntil([&] { return device.GetCurrentState() != state; })) {
                    return;
                }
                break;
        }
    }
}

/// Runs Device::RunStateMachine() in a thread. On destruction, drives the
/// device to Exiting and joins the thread.
class StateMachineThread
{
  public:
    explicit StateMachineThread(Device& device)
        : fDevice(device)
        , fThread(&Device::RunStateMachine, &device)
    {}

    StateMachineThread(const StateMachineThread&) = delete;
    StateMachineThread(StateMachineThread&&) = delete;
    StateMachineThread& operator=(const StateMachineThread&) = delete;
    StateMachineThread& operator=(StateMachineThread&&) = delete;

    ~StateMachineThread() { Join(); }

    void Join()
    {
        if (fThread.joinable()) {
            Shutdown(fDevice);
            fThread.join();
        }
    }

  private:
    Device& fDevice;
    std::thread fThread;
};

}

#endif   // FAIR_MQ_TEST_CONTROL_H
