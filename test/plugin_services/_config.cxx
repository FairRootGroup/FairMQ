/********************************************************************************
 *    Copyright (C) 2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "Fixture.h"
#include <fairmq/Tools.h>
#include <algorithm>

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

TEST_F(PluginServices, KeyDiscovery)
{
    mConfig.SetValue("foo", 0);

    auto keys(mServices.GetPropertyKeys());

    EXPECT_TRUE(find(keys.begin(), keys.end(), "foo") != keys.end());
}

TEST_F(PluginServices, ConfigCallbacks)
{
    mServices.SubscribeToPropertyChange<string>("test", [](const string& key, string value) {
        if (key == "chans.data.0.address") { ASSERT_EQ(value, "tcp://localhost:4321"); }
    });

    mServices.SubscribeToPropertyChange<int>("test", [](const string& key, int /*value*/) {
        if (key == "chans.data.0.rcvBufSize") {
            FAIL(); // should not be called because we unsubscribed
        }
    });

    mServices.SubscribeToPropertyChange<double>("test", [](const string& key, double value) {
        if (key == "data-rate") { ASSERT_EQ(value, 0.9); }
    });

    mServices.SubscribeToDeviceStateChange("test",[&](DeviceState newState){
        switch (newState) {
        case DeviceState::InitializingDevice:
            mServices.SetProperty<string>("chans.data.0.address", "tcp://localhost:4321");
            mServices.SetProperty<int>("chans.data.0.rcvBufSize", 100);
            mServices.SetProperty<double>("data-rate", 0.9);
            break;
        default:
            break;
        }
    });

    mServices.UnsubscribeFromPropertyChange<int>("test");
}

} /* namespace */
