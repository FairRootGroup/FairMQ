/********************************************************************************
 *    Copyright (C) 2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_TEST_FIXTURE
#define FAIR_MQ_TEST_FIXTURE

#include <gtest/gtest.h>
#include <fairmq/PluginServices.h>
#include <FairMQDevice.h>
#include <options/FairMQProgOptions.h>
#include <memory>

namespace fair
{
namespace mq
{
namespace test
{

inline auto control(std::shared_ptr<FairMQDevice> device) -> void
{
    for (const auto event : {
        FairMQDevice::INIT_DEVICE,
        FairMQDevice::RESET_DEVICE,
        FairMQDevice::END,
    }) {
        device->ChangeState(event);
        if (event != FairMQDevice::END) device->WaitForEndOfState(event);
    }
}

struct PluginServices : ::testing::Test {
    PluginServices()
    : mDevice{std::make_shared<FairMQDevice>()}
    , mServices{&mConfig, mDevice}
    {
        mDevice->SetTransport("zeromq");
    }

    ~PluginServices()
    {
        if(mDevice->GetCurrentState() == FairMQDevice::IDLE) control(mDevice);
    }

    FairMQProgOptions mConfig;
    std::shared_ptr<FairMQDevice> mDevice;
    fair::mq::PluginServices mServices;
};

} /* namespace test */
} /* namespace mq */
} /* namespace fair */

#endif /* FAIR_MQ_TEST_FIXTURE */
