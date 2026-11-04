/********************************************************************************
 *    Copyright (C) 2026 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <helper/Devices.h>

#include <gtest/gtest.h>

#include <csignal>
#include <ostream>
#include <string>

namespace
{

using namespace std;
using namespace fair::mq::test;

// The StateAction device with an id that selects no state: it does nothing.
constexpr auto kIdleDevice = "waitfor_None_";

TEST(DeviceRunner, PrintOptions)
{
    auto const results = RunDevices({{kIdleDevice, {}, "--print-options"}}, RunOptions("shmem"));
    auto const& result = results.at(0);
    EXPECT_TRUE(ExitedWith(result, 0, string("id:") + kIdleDevice + ":string:Device ID."));
    EXPECT_TRUE(ExitedWith(result, 0, "transport:shmem:string:"));
    EXPECT_TRUE(ExitedWith(result, 0, "control:static:string:"));
    // The device does not start.
    EXPECT_EQ(result.output.find("INITIALIZING DEVICE"), string::npos) << result.output;
}

TEST(DeviceRunner, PrintChannels)
{
    auto const results = RunDevices({{kIdleDevice, {}, "--print-channels"}});
    auto const& result = results.at(0);
    EXPECT_TRUE(ExitedWith(result, 0, "data:1:1"));
    EXPECT_TRUE(ExitedWith(result, 0, "fan:2:4"));
}

TEST(DeviceRunner, Version)
{
    auto const results = RunDevices({{kIdleDevice, {}, "--version"}});
    EXPECT_TRUE(ExitedWith(results.at(0), 0, "FairMQ version: "));
    EXPECT_TRUE(ExitedWith(results.at(0), 0, "User device version: "));
}

// PrintChannels and Version must not start the device.
TEST(DeviceRunner, PrintChannelsAndVersionDoNotStartTheDevice)
{
    for (auto const* option : {"--print-channels", "--version"}) {
        auto const results = RunDevices({{kIdleDevice, {}, option}});
        EXPECT_TRUE(ExitedWith(results.at(0), 0)) << option;
        EXPECT_EQ(results.at(0).output.find("INITIALIZING DEVICE"), string::npos) << option;
    }
}

struct KeyParam
{
    char key;
    string expected;
};

auto operator<<(std::ostream& os, KeyParam const& param) -> std::ostream&
{
    return os << "key '" << param.key << "'";
}

class InteractiveKey : public testing::TestWithParam<KeyParam>
{};

// One key, then SIGINT ends the device (see the defect below).
TEST_P(InteractiveKey, PrintsItsAction)
{
    RunOptions options;
    options.control = "interactive";
    options.input = string(1, GetParam().key);
    options.signal = SIGINT;
    auto const results = RunDevices({{kIdleDevice}}, options);
    EXPECT_TRUE(ExitedWith(results.at(0), 0, GetParam().expected));
}

INSTANTIATE_TEST_SUITE_P(, InteractiveKey,
                         testing::Values(KeyParam{'h', "--> [h] help"},
                                         KeyParam{'c', "check current state"},
                                         KeyParam{'p', "--> [p] print number of connected peers"},
                                         KeyParam{'k', "--> [k] increase log severity"},
                                         KeyParam{'z', "Invalid input: [z]"}),
                         [](testing::TestParamInfo<KeyParam> const& p) { return string(1, p.param.key); });

// Known defect: the control plugin polls the stdin file descriptor before it
// reads the next key from std::cin. Keys that arrive in one read (e.g. "cq"
// from a pipe) stay in the std::cin buffer, and the plugin never handles them.
// Enable this test with the fix.
TEST(ControlPlugin, DISABLED_SeveralKeysInOneInput)
{
    RunOptions options;
    options.control = "interactive";
    options.input = "cq";
    auto const results = RunDevices({{kIdleDevice}}, options);
    EXPECT_TRUE(ExitedWith(results.at(0), 0, "check current state"));
    EXPECT_TRUE(ExitedWith(results.at(0), 0, "--> [q] end"));
}

} // namespace
