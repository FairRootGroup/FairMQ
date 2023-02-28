/********************************************************************************
 * Copyright (C) 2017-2023 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_TEST_FIXTURE
#define FAIR_MQ_TEST_FIXTURE

#include "../helper/ControlDevice.h"

#include <fairmq/PluginServices.h>
#include <fairmq/Device.h>
#include <fairmq/ProgOptions.h>

#include <gtest/gtest.h>

#include <memory>
#include <thread>

namespace fair::mq::test
{

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

    ~PluginServices() override
    {
        if (mDevice.GetCurrentState() == State::Idle) {
            Control(mDevice, Cycle::ToDeviceReadyAndBack);
        }
        if (fRunStateMachineThread.joinable()) {
            fRunStateMachineThread.join();
        }
    }

    ProgOptions mConfig;
    Device mDevice;
    mq::PluginServices mServices;
    std::thread fRunStateMachineThread;
};

} // namespace fair::mq::test

#endif /* FAIR_MQ_TEST_FIXTURE */
