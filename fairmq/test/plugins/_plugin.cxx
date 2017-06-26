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
#include <FairMQDevice.h>
#include <options/FairMQProgOptions.h>
#include <sstream>
#include <string>
#include <vector>

namespace
{

using namespace std;
using namespace fair::mq;

auto control(FairMQDevice& device) -> void
{
    device.SetTransport("zeromq");
    for (const auto event : {
        FairMQDevice::INIT_DEVICE,
        FairMQDevice::RESET_DEVICE,
        FairMQDevice::END,
    }) {
        device.ChangeState(event);
        if (event != FairMQDevice::END) device.WaitForEndOfState(event);
    }
}

TEST(Plugin, Operators)
{
    FairMQProgOptions config{};
    FairMQDevice device{};
    PluginServices services{config, device};
    auto p1 = Plugin{"dds", {1, 0, 0}, "Foo Bar <foo.bar@test.net>", "https://git.test.net/dds.git", services};
    auto p2 = Plugin{"dds", {1, 0, 0}, "Foo Bar <foo.bar@test.net>", "https://git.test.net/dds.git", services};
    auto p3 = Plugin{"file", {1, 0, 0}, "Foo Bar <foo.bar@test.net>", "https://git.test.net/file.git", services};
    EXPECT_EQ(p1, p2);
    EXPECT_NE(p1, p3);
    control(device);
}

TEST(Plugin, OstreamOperators)
{
    FairMQProgOptions config{};
    FairMQDevice device{};
    PluginServices services{config, device};
    auto p1 = Plugin{"dds", {1, 0, 0}, "Foo Bar <foo.bar@test.net>", "https://git.test.net/dds.git", services};
    stringstream ss;
    ss << p1;
    EXPECT_EQ(ss.str(), string{"'dds', version '1.0.0', maintainer 'Foo Bar <foo.bar@test.net>', homepage 'https://git.test.net/dds.git'"});
    control(device);
}

TEST(PluginVersion, Operators)
{
    struct Plugin::Version v1{1, 0, 0};
    struct Plugin::Version v2{1, 0, 0};
    struct Plugin::Version v3{1, 2, 0};
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
