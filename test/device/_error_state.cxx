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

void RunErrorStateIn(const std::string& state, const std::string& input = "")
{
    size_t session{fair::mq::tools::UuidHash()};

    execute_result result{"", 100};
    thread device_thread([&]() {
        stringstream cmd;
        cmd << runTestDevice
            << " --id error_state_" << state << "_"
            << " --control " << ((input == "") ? "static" : "interactive")
            << " --session " << session
            << " --color false";
        result = execute(cmd.str(), "[ErrorFound IN " + state + "]", input);
    });

    device_thread.join();

    ASSERT_NE(std::string::npos, result.console_out.find("going to change to Error state from " + state + "()"));

    exit(result.exit_code);
}

TEST(ErrorState, static_InInit)
{
    EXPECT_EXIT(RunErrorStateIn("Init"), ::testing::ExitedWithCode(1), "");
}
TEST(ErrorState, static_InBind)
{
    EXPECT_EXIT(RunErrorStateIn("Bind"), ::testing::ExitedWithCode(1), "");
}
TEST(ErrorState, static_InConnect)
{
    EXPECT_EXIT(RunErrorStateIn("Connect"), ::testing::ExitedWithCode(1), "");
}
TEST(ErrorState, static_InInitTask)
{
    EXPECT_EXIT(RunErrorStateIn("InitTask"), ::testing::ExitedWithCode(1), "");
}
TEST(ErrorState, static_InPreRun)
{
    EXPECT_EXIT(RunErrorStateIn("PreRun"), ::testing::ExitedWithCode(1), "");
}
TEST(ErrorState, static_InRun)
{
    EXPECT_EXIT(RunErrorStateIn("Run"), ::testing::ExitedWithCode(1), "");
}
TEST(ErrorState, static_InPostRun)
{
    EXPECT_EXIT(RunErrorStateIn("PostRun"), ::testing::ExitedWithCode(1), "");
}
TEST(ErrorState, static_InResetTask)
{
    EXPECT_EXIT(RunErrorStateIn("ResetTask"), ::testing::ExitedWithCode(1), "");
}
TEST(ErrorState, static_InReset)
{
    EXPECT_EXIT(RunErrorStateIn("Reset"), ::testing::ExitedWithCode(1), "");
}
TEST(ErrorState, interactive_InInit)
{
    EXPECT_EXIT(RunErrorStateIn("Init", "q"), ::testing::ExitedWithCode(1), "");
}
TEST(ErrorState, interactive_InBind)
{
    EXPECT_EXIT(RunErrorStateIn("Bind", "q"), ::testing::ExitedWithCode(1), "");
}
TEST(ErrorState, interactive_InConnect)
{
    EXPECT_EXIT(RunErrorStateIn("Connect", "q"), ::testing::ExitedWithCode(1), "");
}
TEST(ErrorState, interactive_InInitTask)
{
    EXPECT_EXIT(RunErrorStateIn("InitTask", "q"), ::testing::ExitedWithCode(1), "");
}
TEST(ErrorState, interactive_InPreRun)
{
    EXPECT_EXIT(RunErrorStateIn("PreRun", "q"), ::testing::ExitedWithCode(1), "");
}
TEST(ErrorState, interactive_InRun)
{
    EXPECT_EXIT(RunErrorStateIn("Run", "q"), ::testing::ExitedWithCode(1), "");
}
TEST(ErrorState, interactive_InPostRun)
{
    EXPECT_EXIT(RunErrorStateIn("PostRun", "q"), ::testing::ExitedWithCode(1), "");
}
TEST(ErrorState, interactive_InResetTask)
{
    EXPECT_EXIT(RunErrorStateIn("ResetTask", "q"), ::testing::ExitedWithCode(1), "");
}
TEST(ErrorState, interactive_InReset)
{
    EXPECT_EXIT(RunErrorStateIn("Reset", "q"), ::testing::ExitedWithCode(1), "");
}
TEST(ErrorState, interactive_invalid_InInit)
{
    EXPECT_EXIT(RunErrorStateIn("Init", "_"), ::testing::ExitedWithCode(1), "");
}
TEST(ErrorState, interactive_invalid_InBind)
{
    EXPECT_EXIT(RunErrorStateIn("Bind", "_"), ::testing::ExitedWithCode(1), "");
}
TEST(ErrorState, interactive_invalid_InConnect)
{
    EXPECT_EXIT(RunErrorStateIn("Connect", "_"), ::testing::ExitedWithCode(1), "");
}
TEST(ErrorState, interactive_invalid_InInitTask)
{
    EXPECT_EXIT(RunErrorStateIn("InitTask", "_"), ::testing::ExitedWithCode(1), "");
}
TEST(ErrorState, interactive_invalid_InPreRun)
{
    EXPECT_EXIT(RunErrorStateIn("PreRun", "_"), ::testing::ExitedWithCode(1), "");
}
TEST(ErrorState, interactive_invalid_InRun)
{
    EXPECT_EXIT(RunErrorStateIn("Run", "_"), ::testing::ExitedWithCode(1), "");
}
TEST(ErrorState, interactive_invalid_InPostRun)
{
    EXPECT_EXIT(RunErrorStateIn("PostRun", "_"), ::testing::ExitedWithCode(1), "");
}

} // namespace
