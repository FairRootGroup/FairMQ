/********************************************************************************
 * Copyright (C) 2014-2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <helper/Devices.h>

#include <gtest/gtest.h>

#include <csignal>
#include <string>

namespace
{

using namespace std;
using namespace fair::mq::test;

void RunWaitFor(string const& state, string const& control)
{
    RunOptions options;
    options.control = control;
    options.signal = SIGINT;
    options.extraArgs = "--severity debug";
    auto const results = RunDevices({{"waitfor_" + state + "_"}}, options);

    EXPECT_TRUE(ExitedWith(results.at(0), 0, "Sleeping Done. Interrupted."));
}

TEST(WaitFor, static_InPreRun)
{
    RunWaitFor("PreRun", "static");
}

TEST(WaitFor, static_InRun)
{
    RunWaitFor("Run", "static");
}

TEST(WaitFor, static_InPostRun)
{
    RunWaitFor("PostRun", "static");
}

TEST(WaitFor, interactive_InPreRun)
{
    RunWaitFor("PreRun", "interactive");
}

TEST(WaitFor, interactive_InRun)
{
    RunWaitFor("Run", "interactive");
}

TEST(WaitFor, interactive_InPostRun)
{
    RunWaitFor("PostRun", "interactive");
}

} // namespace
