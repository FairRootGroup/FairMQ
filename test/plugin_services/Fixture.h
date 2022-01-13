/********************************************************************************
 *    Copyright (C) 2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_TEST_FIXTURE
#define FAIR_MQ_TEST_FIXTURE

#include <fairmq/PluginServices.h>
#include <fairmq/Device.h>
#include <fairmq/ProgOptions.h>

#include <gtest/gtest.h>

#include <memory>
#include <thread>

namespace fair::mq::test
{

inline auto control(fair::mq::Device& device) -> void
{
    device.ChangeState(fair::mq::Transition::InitDevice);
    device.WaitForState(fair::mq::State::InitializingDevice);
    device.ChangeState(fair::mq::Transition::CompleteInit);
    device.WaitForState(fair::mq::State::Initialized);
    device.ChangeState(fair::mq::Transition::Bind);
    device.WaitForState(fair::mq::State::Bound);
    device.ChangeState(fair::mq::Transition::Connect);
    device.WaitForState(fair::mq::State::DeviceReady);
    device.ChangeState(fair::mq::Transition::ResetDevice);
    device.WaitForState(fair::mq::State::Idle);

    device.ChangeState(fair::mq::Transition::End);
}

struct PluginServices : ::testing::Test {
    PluginServices()
        : mConfig()
        , mDevice()
        , mServices(mConfig, mDevice)
        , fRunStateMachineThread()
    {
        fRunStateMachineThread = std::thread(&fair::mq::Device::RunStateMachine, &mDevice);
        mDevice.SetTransport("zeromq");
    }

    ~PluginServices()
    {
        if (mDevice.GetCurrentState() == fair::mq::State::Idle) control(mDevice);
        if (fRunStateMachineThread.joinable()) {
            fRunStateMachineThread.join();
        }
    }

    fair::mq::ProgOptions mConfig;
    fair::mq::Device mDevice;
    fair::mq::PluginServices mServices;
    std::thread fRunStateMachineThread;
};

} // namespace fair::mq::test

#endif /* FAIR_MQ_TEST_FIXTURE */
