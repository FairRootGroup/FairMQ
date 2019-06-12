/********************************************************************************
 *    Copyright (C) 2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "Fixture.h"
#include <fairmq/Tools.h>
#include <fairmq/PropertyOutput.h>
#include <algorithm>

struct MyClass
{
    MyClass() : msg() {}
    MyClass(const std::string& a) : msg(a) {}
    MyClass& operator=(const MyClass& b) { msg = b.msg; return *this; };
    std::string msg;
};

std::ostream& operator<<(std::ostream& os, const MyClass& b)
{
    os << b.msg;
    return os;
}

std::istream& operator>>(std::istream& in, MyClass& b)
{
    std::string val;
    in >> val;
    b.msg = val;
    return in;
}

namespace
{

using namespace std;
using namespace fair::mq;
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
                EXPECT_EQ(mServices.GetPropertyAsString("blubb"), tools::ToString(42));
                break;
            default:
                break;
        }
    });
}

TEST_F(PluginServices, KeyDiscovery)
{
    mConfig.SetProperty("foo", 0);

    auto keys(mServices.GetPropertyKeys());

    EXPECT_TRUE(find(keys.begin(), keys.end(), "foo") != keys.end());
}

TEST_F(PluginServices, ConfigCallbacks)
{
    mServices.SubscribeToPropertyChange<string>("test", [](const string& key, string value) {
        if (key == "chans.data.0.address") { ASSERT_EQ(value, "tcp://localhost:4321"); }
        if (key == "custom1") { ASSERT_EQ(value, "test1"); }
        if (key == "custom2") { ASSERT_EQ(value, "test2"); }
    });

    mServices.SubscribeToPropertyChange<int>("test", [](const string& key, int /*value*/) {
        if (key == "chans.data.0.rcvBufSize") {
            FAIL(); // should not be called because we unsubscribed
        }
    });

    mServices.SubscribeToPropertyChange<double>("test", [](const string& key, double value) {
        if (key == "data-rate") { ASSERT_EQ(value, 0.9); }
    });

    mServices.SetProperty<string>("chans.data.0.address", "tcp://localhost:4321");
    mServices.SetProperty<double>("data-rate", 0.9);

    mServices.UnsubscribeFromPropertyChange<int>("test");

    mServices.SetProperty<int>("chans.data.0.rcvBufSize", 100);
}

TEST_F(PluginServices, Accessors)
{
    // try adding properties in bulk
    Properties properties;
    properties.emplace("custom1", "test1");
    properties.emplace("custom2", "test2");
    mServices.SetProperties(properties);

    // an existing property should exist
    ASSERT_EQ(mServices.PropertyExists("custom1"), true);
    // a not existing property should not exists
    ASSERT_EQ(mServices.PropertyExists("custom3"), false);
    // updating a not existing property should fail
    ASSERT_EQ(mServices.UpdateProperty("custom3", 12345), false);
    mServices.SetProperty("custom3", 12345);
    // updating an existing existing property should succeed
    ASSERT_EQ(mServices.UpdateProperty("custom3", 67890), true);
    // updated property should return the correct value
    ASSERT_EQ(mServices.GetProperty<int>("custom3"), 67890);
    // asking for non existing property should return the fallback if it is provided
    ASSERT_EQ(mServices.GetProperty("custom4", 1005), 1005);

    properties = mServices.GetPropertiesStartingWith("custom");
    // bulk operation should return correct number elements
    ASSERT_EQ(properties.size(), 3);
    mServices.SetProperty("newcustom", 12345);
    properties = mServices.GetProperties(".*custom.*");
    // bulk operation should return correct number elements
    ASSERT_EQ(properties.size(), 4);

    // the container returned by the bulk operation should contain correct values
    ASSERT_EQ(boost::any_cast<const char*>(properties.at("custom1")), "test1");
    ASSERT_EQ(boost::any_cast<const char*>(properties.at("custom2")), "test2");
    ASSERT_EQ(boost::any_cast<int>(properties.at("custom3")), 67890);
    ASSERT_EQ(boost::any_cast<int>(properties.at("newcustom")), 12345);

    properties.at("custom3") = 11111;
    mServices.UpdateProperties(properties);

    // bulk update should update values of all properties, but only if all of them exist
    ASSERT_EQ(boost::any_cast<int>(properties.at("custom3")), 11111);

    properties.at("custom3") = 22222;
    properties.emplace("custom4", 17);
    // bulk update should fail if any of the properties do not exist in the container ...
    ASSERT_EQ(mServices.UpdateProperties(properties), false);
    // ... all the values should remain unchanged
    ASSERT_EQ(mServices.GetProperty<int>("custom3"), 11111);

    mServices.DeleteProperty("custom3");
    // property should no longer exist after deletion
    ASSERT_EQ(mServices.PropertyExists("custom3"), false);
    // accessing this property should throw an exception
    ASSERT_THROW(mServices.GetProperty<int>("custom3"), fair::mq::PluginServices::PropertyNotFoundError);

    mServices.SetProperty("customType", MyClass("message"));
    // without adding custom type information, proper string value will not be returned
    ASSERT_NE(mServices.GetPropertyAsString("customType"), "message");
    ASSERT_EQ(mServices.GetPropertyAsString("customType"), "[unidentified_type]");
    PropertyHelper::AddType<MyClass>();
    // after calling AddType proper string value should be returned
    ASSERT_EQ(mServices.GetPropertyAsString("customType"), "message");
}

} /* namespace */
