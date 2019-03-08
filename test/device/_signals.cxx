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
#include <fairmq/Tools.h>

#include <string>
#include <thread>
#include <iostream>

namespace
{

using namespace std;
using namespace fair::mq::test;
using namespace fair::mq::tools;

void RunSignalIn(const std::string& state, const std::string& input = "")
{
    size_t session{fair::mq::tools::UuidHash()};

    execute_result result{"", 100};
    thread device_thread([&]() {
        stringstream cmd;
        cmd << runTestDevice
            << " --id signals_" << state << "_"
            << " --control " << ((input == "") ? "static" : "interactive")
            << " --session " << session
            << " --color false";
        result = execute(cmd.str(), "[SIGINT IN " + state + "]", input);
    });

    device_thread.join();

    ASSERT_NE(std::string::npos, result.console_out.find("raising SIGINT from " + state + "()"));

    exit(result.exit_code);
}

TEST(Signal_SIGINT, static_InInit)
{
    EXPECT_EXIT(RunSignalIn("Init"), ::testing::ExitedWithCode(0), "");
}
TEST(Signal_SIGINT, static_InBind)
{
    EXPECT_EXIT(RunSignalIn("Bind"), ::testing::ExitedWithCode(0), "");
}
TEST(Signal_SIGINT, static_InConnect)
{
    EXPECT_EXIT(RunSignalIn("Connect"), ::testing::ExitedWithCode(0), "");
}
TEST(Signal_SIGINT, static_InInitTask)
{
    EXPECT_EXIT(RunSignalIn("InitTask"), ::testing::ExitedWithCode(0), "");
}
TEST(Signal_SIGINT, static_InPreRun)
{
    EXPECT_EXIT(RunSignalIn("PreRun"), ::testing::ExitedWithCode(0), "");
}
TEST(Signal_SIGINT, static_InRun)
{
    EXPECT_EXIT(RunSignalIn("Run"), ::testing::ExitedWithCode(0), "");
}
TEST(Signal_SIGINT, static_InPostRun)
{
    EXPECT_EXIT(RunSignalIn("PostRun"), ::testing::ExitedWithCode(0), "");
}
TEST(Signal_SIGINT, static_InResetTask)
{
    EXPECT_EXIT(RunSignalIn("ResetTask"), ::testing::ExitedWithCode(0), "");
}
TEST(Signal_SIGINT, static_InReset)
{
    EXPECT_EXIT(RunSignalIn("Reset"), ::testing::ExitedWithCode(0), "");
}
TEST(Signal_SIGINT, interactive_InInit)
{
    EXPECT_EXIT(RunSignalIn("Init", "q"), ::testing::ExitedWithCode(0), "");
}
TEST(Signal_SIGINT, interactive_InBind)
{
    EXPECT_EXIT(RunSignalIn("Bind", "q"), ::testing::ExitedWithCode(0), "");
}
TEST(Signal_SIGINT, interactive_InConnect)
{
    EXPECT_EXIT(RunSignalIn("Connect", "q"), ::testing::ExitedWithCode(0), "");
}
TEST(Signal_SIGINT, interactive_InInitTask)
{
    EXPECT_EXIT(RunSignalIn("InitTask", "q"), ::testing::ExitedWithCode(0), "");
}
TEST(Signal_SIGINT, interactive_InPreRun)
{
    EXPECT_EXIT(RunSignalIn("PreRun", "q"), ::testing::ExitedWithCode(0), "");
}
TEST(Signal_SIGINT, interactive_InRun)
{
    EXPECT_EXIT(RunSignalIn("Run", "q"), ::testing::ExitedWithCode(0), "");
}
TEST(Signal_SIGINT, interactive_InPostRun)
{
    EXPECT_EXIT(RunSignalIn("PostRun", "q"), ::testing::ExitedWithCode(0), "");
}
TEST(Signal_SIGINT, interactive_InResetTask)
{
    EXPECT_EXIT(RunSignalIn("ResetTask", "q"), ::testing::ExitedWithCode(0), "");
}
TEST(Signal_SIGINT, interactive_InReset)
{
    EXPECT_EXIT(RunSignalIn("Reset", "q"), ::testing::ExitedWithCode(0), "");
}
TEST(Signal_SIGINT, interactive_invalid_InInit)
{
    EXPECT_EXIT(RunSignalIn("Init", "_"), ::testing::ExitedWithCode(0), "");
}
TEST(Signal_SIGINT, interactive_invalid_InBind)
{
    EXPECT_EXIT(RunSignalIn("Bind", "_"), ::testing::ExitedWithCode(0), "");
}
TEST(Signal_SIGINT, interactive_invalid_InConnect)
{
    EXPECT_EXIT(RunSignalIn("Connect", "_"), ::testing::ExitedWithCode(0), "");
}
TEST(Signal_SIGINT, interactive_invalid_InInitTask)
{
    EXPECT_EXIT(RunSignalIn("InitTask", "_"), ::testing::ExitedWithCode(0), "");
}
TEST(Signal_SIGINT, interactive_invalid_InPreRun)
{
    EXPECT_EXIT(RunSignalIn("PreRun", "_"), ::testing::ExitedWithCode(0), "");
}
TEST(Signal_SIGINT, interactive_invalid_InRun)
{
    EXPECT_EXIT(RunSignalIn("Run", "_"), ::testing::ExitedWithCode(0), "");
}
TEST(Signal_SIGINT, interactive_invalid_InPostRun)
{
    EXPECT_EXIT(RunSignalIn("PostRun", "_"), ::testing::ExitedWithCode(0), "");
}

} // namespace
