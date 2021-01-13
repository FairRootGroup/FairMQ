/********************************************************************************
 *    Copyright (C) 2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "runner.h"

#include <gtest/gtest.h>
#include <boost/process.hpp>
#include <fairmq/tools/Unique.h>
#include <fairmq/tools/Process.h>

#include <string>
#include <thread>
#include <iostream>

namespace
{

using namespace std;
using namespace fair::mq::test;
using namespace fair::mq::tools;

void RunSignalIn(const string& state, const string& control, const string& input = "")
{
    size_t session{fair::mq::tools::UuidHash()};

    execute_result result{"", 100};
    thread device_thread([&]() {
        stringstream cmd;
        cmd << runTestDevice
            << " --id signals_" << state << "_"
            << " --control " << control
            << " --session " << session
            << " --color false";
        result = execute(cmd.str(), "[SIGINT IN " + state + "]", input);
    });

    device_thread.join();

    ASSERT_NE(string::npos, result.console_out.find("raising SIGINT from " + state + "()"));

    exit(result.exit_code);
}

TEST(Signal_SIGINT, static_InInit)
{
    EXPECT_EXIT(RunSignalIn("Init", "static"), ::testing::ExitedWithCode(0), "");
}
TEST(Signal_SIGINT, static_InBind)
{
    EXPECT_EXIT(RunSignalIn("Bind", "static"), ::testing::ExitedWithCode(0), "");
}
TEST(Signal_SIGINT, static_InConnect)
{
    EXPECT_EXIT(RunSignalIn("Connect", "static"), ::testing::ExitedWithCode(0), "");
}
TEST(Signal_SIGINT, static_InInitTask)
{
    EXPECT_EXIT(RunSignalIn("InitTask", "static"), ::testing::ExitedWithCode(0), "");
}
TEST(Signal_SIGINT, static_InPreRun)
{
    EXPECT_EXIT(RunSignalIn("PreRun", "static"), ::testing::ExitedWithCode(0), "");
}
TEST(Signal_SIGINT, static_InRun)
{
    EXPECT_EXIT(RunSignalIn("Run", "static"), ::testing::ExitedWithCode(0), "");
}
TEST(Signal_SIGINT, static_InPostRun)
{
    EXPECT_EXIT(RunSignalIn("PostRun", "static"), ::testing::ExitedWithCode(0), "");
}
TEST(Signal_SIGINT, static_InResetTask)
{
    EXPECT_EXIT(RunSignalIn("ResetTask", "static"), ::testing::ExitedWithCode(0), "");
}
TEST(Signal_SIGINT, static_InReset)
{
    EXPECT_EXIT(RunSignalIn("Reset", "static"), ::testing::ExitedWithCode(0), "");
}
TEST(Signal_SIGINT, interactive_InInit)
{
    EXPECT_EXIT(RunSignalIn("Init", "interactive"), ::testing::ExitedWithCode(0), "");
}
TEST(Signal_SIGINT, interactive_InBind)
{
    EXPECT_EXIT(RunSignalIn("Bind", "interactive"), ::testing::ExitedWithCode(0), "");
}
TEST(Signal_SIGINT, interactive_InConnect)
{
    EXPECT_EXIT(RunSignalIn("Connect", "interactive"), ::testing::ExitedWithCode(0), "");
}
TEST(Signal_SIGINT, interactive_InInitTask)
{
    EXPECT_EXIT(RunSignalIn("InitTask", "interactive"), ::testing::ExitedWithCode(0), "");
}
TEST(Signal_SIGINT, interactive_InPreRun)
{
    EXPECT_EXIT(RunSignalIn("PreRun", "interactive"), ::testing::ExitedWithCode(0), "");
}
TEST(Signal_SIGINT, interactive_InRun)
{
    EXPECT_EXIT(RunSignalIn("Run", "interactive"), ::testing::ExitedWithCode(0), "");
}
TEST(Signal_SIGINT, interactive_InPostRun)
{
    EXPECT_EXIT(RunSignalIn("PostRun", "interactive"), ::testing::ExitedWithCode(0), "");
}
TEST(Signal_SIGINT, interactive_InResetTask)
{
    EXPECT_EXIT(RunSignalIn("ResetTask", "interactive", "q"), ::testing::ExitedWithCode(0), "");
}
TEST(Signal_SIGINT, interactive_InReset)
{
    EXPECT_EXIT(RunSignalIn("Reset", "interactive", "q"), ::testing::ExitedWithCode(0), "");
}

} // namespace
