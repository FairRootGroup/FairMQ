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

namespace fair::mq::test
{

struct PluginServices : ::testing::Test {
    PluginServices()
        : mConfig()
        , mDevice()
        , mServices(mConfig, mDevice)
        , fStateMachineThread(mDevice)
    {
        mDevice.SetTransport("zeromq");
    }

    ~PluginServices() override
    {
        if (mDevice.GetCurrentState() == State::Idle) {
            Control(mDevice, Cycle::ToDeviceReadyAndBack);
        }
        // fStateMachineThread drives the device to Exiting from any other state
    }

    ProgOptions mConfig;
    Device mDevice;
    mq::PluginServices mServices;
    StateMachineThread fStateMachineThread;
};

} // namespace fair::mq::test

#endif /* FAIR_MQ_TEST_FIXTURE */
