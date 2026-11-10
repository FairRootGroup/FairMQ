/********************************************************************************
 *    Copyright (C) 2026 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_TEST_DEVICES_H
#define FAIR_MQ_TEST_DEVICES_H

#include "devices/TestDevice.h"
#include "../runner.h"

#include <fairmq/tools/Process.h>
#include <fairmq/tools/Strings.h>
#include <fairmq/tools/Unique.h>

#include <gtest/gtest.h>

#include <chrono>
#include <cstddef>
#include <sstream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace fair::mq::test
{

/// One runTestDevice process.
struct DeviceSpec
{
    DeviceSpec(std::string id_, std::string channelConfig_ = {}, std::string extraArgs_ = {})
        : id(std::move(id_))
        , channelConfig(std::move(channelConfig_))
        , extraArgs(std::move(extraArgs_))
    {}

    std::string id;             ///< selects the device class, see runTestDevice.cxx
    std::string channelConfig;  ///< value of --channel-config (empty: none)
    std::string extraArgs;      ///< more command line arguments for this device
};

/// Settings for all devices of one RunDevices() call.
struct RunOptions
{
    explicit RunOptions(std::string transport_ = "zeromq", std::string extraArgs_ = {})
        : transport(std::move(transport_))
        , extraArgs(std::move(extraArgs_))
    {}

    std::string transport;
    std::string extraArgs;           ///< more command line arguments for each device
    std::string control = "static";
    std::string input;               ///< sent to stdin of each device after 1 s
    int signal = -1;                 ///< sent to each device after 2 s (-1: none)
    std::chrono::milliseconds timeout = std::chrono::seconds(15);
};

struct DeviceResult
{
    std::string id;
    std::string output;
    int exitCode = -1;
};

/// Run the devices at the same time, in one session, and wait for all of
/// them. A device that runs longer than options.timeout is killed.
inline auto RunDevices(std::vector<DeviceSpec> const& devices, RunOptions const& options = RunOptions())
    -> std::vector<DeviceResult>
{
    auto const session = std::to_string(tools::UuidHash());
    std::vector<DeviceResult> results(devices.size());
    std::vector<std::thread> threads;
    threads.reserve(devices.size());

    for (std::size_t i = 0; i < devices.size(); ++i) {
        threads.emplace_back([&, i] {
            auto const& device = devices[i];
            std::stringstream cmd;
            cmd << runTestDevice
                << " --id " << device.id
                << " --transport " << options.transport
                << " --control " << options.control
                << " --session " << session
                << " --shm-segment-size 100000000"
                << " --shm-monitor true"
                << " --color false";
            if (!options.extraArgs.empty()) {
                cmd << " " << options.extraArgs;
            }
            if (!device.extraArgs.empty()) {
                cmd << " " << device.extraArgs;
            }
            if (!device.channelConfig.empty()) {
                cmd << " --channel-config " << device.channelConfig;
            }
            auto const res = tools::execute(cmd.str(),
                                            tools::ToString("[", device.id, "] "),
                                            options.input,
                                            options.signal,
                                            options.timeout);
            results[i] = DeviceResult{device.id, res.console_out, res.exit_code};
        });
    }
    for (auto& t : threads) {
        t.join();
    }
    return results;
}

/// Check that the device exited with exitCode and that its output contains text.
inline auto ExitedWith(DeviceResult const& result, int exitCode, std::string const& text = "")
    -> ::testing::AssertionResult
{
    if (result.exitCode != exitCode) {
        return ::testing::AssertionFailure() << "device '" << result.id << "' exited with "
                                             << result.exitCode << ", expected " << exitCode
                                             << ", output:\n" << result.output;
    }
    if (!text.empty() && result.output.find(text) == std::string::npos) {
        return ::testing::AssertionFailure() << "output of device '" << result.id << "' has no '"
                                             << text << "', output:\n" << result.output;
    }
    return ::testing::AssertionSuccess();
}

/// Check that the device exited with 0 and reported success on the transport.
inline auto Succeeded(DeviceResult const& result, std::string const& transport)
    -> ::testing::AssertionResult
{
    return ExitedWith(result, 0, std::string(kSuccessMarker).append(transport));
}

} // namespace fair::mq::test

#endif /* FAIR_MQ_TEST_DEVICES_H */
