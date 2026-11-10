/********************************************************************************
 * Copyright (C) 2014-2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <helper/Devices.h>
#include <helper/StateParam.h>

#include <gtest/gtest.h>

#include <csignal>
#include <string>

namespace
{

using namespace std;
using namespace fair::mq::test;

using WaitFor = StateTest;

TEST_P(WaitFor, Interrupt)
{
    RunOptions options;
    options.control = GetParam().control;
    options.signal = SIGINT;
    options.extraArgs = "--severity debug";
    auto const results = RunDevices({{"waitfor_" + GetParam().state + "_"}}, options);

    EXPECT_TRUE(ExitedWith(results.at(0), 0, "Sleeping Done. Interrupted."));
}

// The device waits (and SIGINT interrupts the wait) only in these states.
INSTANTIATE_TEST_SUITE_P(, WaitFor, WithControls({"PreRun", "Run", "PostRun"}), StateParamName());

} // namespace
