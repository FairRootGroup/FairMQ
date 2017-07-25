/********************************************************************************
 *    Copyright (C) 2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "Fixture.h"
#include <fairmq/Tools.h>

namespace
{

using namespace std;
using fair::mq::test::PluginServices;
using DeviceState = fair::mq::PluginServices::DeviceState;
using DeviceStateTransition = fair::mq::PluginServices::DeviceStateTransition;

TEST_F(PluginServices, ConfigSynchronous)
{
    mServices.SubscribeToDeviceStateChange("test",[&](DeviceState newState){
        switch (newState) {
        case DeviceState::InitializingDevice:
            mServices.SetProperty<int>("blubb", 42);
            break;
        case DeviceState::DeviceReady:
            EXPECT_EQ(mServices.GetProperty<int>("blubb"), 42);
            EXPECT_EQ(mServices.GetPropertyAsString("blubb"), fair::mq::tools::ToString(42));
            break;
        default:
            break;
        }
    });
}

TEST_F(PluginServices, ConfigInvalidStateError)
{
    mServices.SubscribeToDeviceStateChange("test",[&](DeviceState newState){
        switch (newState) {
        case DeviceState::DeviceReady:
            ASSERT_THROW(mServices.SetProperty<int>("blubb", 42), fair::mq::PluginServices::InvalidStateError);
            break;
        default:
            break;
        }
    });
}

} /* namespace */
