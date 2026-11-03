/********************************************************************************
 *    Copyright (C) 2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <helper/Devices.h>

#include <gtest/gtest.h>

#include <string>

namespace
{

using namespace std;
using namespace fair::mq::test;

void RunSignalIn(string const& state, string const& control, string const& input = "")
{
    RunOptions options;
    options.control = control;
    options.input = input;
    auto const results = RunDevices({{"signals_" + state + "_"}}, options);

    EXPECT_TRUE(ExitedWith(results.at(0), 0, "raising SIGINT from " + state + "()"));
}

TEST(Signal_SIGINT, static_InInit)
{
    RunSignalIn("Init", "static");
}

TEST(Signal_SIGINT, static_InBind)
{
    RunSignalIn("Bind", "static");
}

TEST(Signal_SIGINT, static_InConnect)
{
    RunSignalIn("Connect", "static");
}

TEST(Signal_SIGINT, static_InInitTask)
{
    RunSignalIn("InitTask", "static");
}

TEST(Signal_SIGINT, static_InPreRun)
{
    RunSignalIn("PreRun", "static");
}

TEST(Signal_SIGINT, static_InRun)
{
    RunSignalIn("Run", "static");
}

TEST(Signal_SIGINT, static_InPostRun)
{
    RunSignalIn("PostRun", "static");
}

TEST(Signal_SIGINT, static_InResetTask)
{
    RunSignalIn("ResetTask", "static");
}

TEST(Signal_SIGINT, static_InReset)
{
    RunSignalIn("Reset", "static");
}

TEST(Signal_SIGINT, interactive_InInit)
{
    RunSignalIn("Init", "interactive");
}

TEST(Signal_SIGINT, interactive_InBind)
{
    RunSignalIn("Bind", "interactive");
}

TEST(Signal_SIGINT, interactive_InConnect)
{
    RunSignalIn("Connect", "interactive");
}

TEST(Signal_SIGINT, interactive_InInitTask)
{
    RunSignalIn("InitTask", "interactive");
}

TEST(Signal_SIGINT, interactive_InPreRun)
{
    RunSignalIn("PreRun", "interactive");
}

TEST(Signal_SIGINT, interactive_InRun)
{
    RunSignalIn("Run", "interactive");
}

TEST(Signal_SIGINT, interactive_InPostRun)
{
    RunSignalIn("PostRun", "interactive");
}

TEST(Signal_SIGINT, interactive_InResetTask)
{
    RunSignalIn("ResetTask", "interactive", "q");
}

TEST(Signal_SIGINT, interactive_InReset)
{
    RunSignalIn("Reset", "interactive", "q");
}

} // namespace
