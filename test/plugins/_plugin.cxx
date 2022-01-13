/********************************************************************************
 *    Copyright (C) 2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <gtest/gtest.h>
#include <fairmq/Plugin.h>
#include <fairmq/PluginServices.h>
#include <fairmq/tools/Version.h>
#include <fairmq/Device.h>
#include <fairmq/ProgOptions.h>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include <thread>

namespace _plugin
{

using namespace std;
using namespace fair::mq;

auto control(Device& device) -> void
{
    device.SetTransport("zeromq");

    device.ChangeState(Transition::InitDevice);
    device.WaitForState(State::InitializingDevice);
    device.ChangeState(Transition::CompleteInit);
    device.WaitForState(State::Initialized);
    device.ChangeState(Transition::Bind);
    device.WaitForState(State::Bound);
    device.ChangeState(Transition::Connect);
    device.WaitForState(State::DeviceReady);
    device.ChangeState(Transition::ResetDevice);
    device.WaitForState(State::Idle);

    device.ChangeState(Transition::End);
}

TEST(Plugin, Operators)
{
    ProgOptions config;
    Device device;
    PluginServices services{config, device};
    Plugin p1{"dds", {1, 0, 0}, "Foo Bar <foo.bar@test.net>", "https://git.test.net/dds.git", &services};
    Plugin p2{"dds", {1, 0, 0}, "Foo Bar <foo.bar@test.net>", "https://git.test.net/dds.git", &services};
    Plugin p3{"file", {1, 0, 0}, "Foo Bar <foo.bar@test.net>", "https://git.test.net/file.git", &services};
    EXPECT_EQ(p1, p2);
    EXPECT_NE(p1, p3);
    thread t(control, std::ref(device));
    device.RunStateMachine();
    if (t.joinable()) {
        t.join();
    }
}

TEST(Plugin, OstreamOperators)
{
    ProgOptions config;
    Device device;
    PluginServices services{config, device};
    Plugin p1{"dds", {1, 0, 0}, "Foo Bar <foo.bar@test.net>", "https://git.test.net/dds.git", &services};
    stringstream ss;
    ss << p1;
    EXPECT_EQ(ss.str(), string{"'dds', version '1.0.0', maintainer 'Foo Bar <foo.bar@test.net>', homepage 'https://git.test.net/dds.git'"});
    thread t(control, std::ref(device));
    device.RunStateMachine();
    if (t.joinable()) {
        t.join();
    }
}

TEST(PluginVersion, Operators)
{
    struct tools::Version v1{1, 0, 0};
    struct tools::Version v2{1, 0, 0};
    struct tools::Version v3{1, 2, 0};
    EXPECT_EQ(v1, v2);
    EXPECT_NE(v1, v3);
    EXPECT_GT(v3, v2);
    EXPECT_LT(v1, v3);
    EXPECT_GE(v3, v2);
    EXPECT_GE(v2, v1);
    EXPECT_LE(v1, v2);
    EXPECT_LE(v2, v3);
}

} /* namespace */
