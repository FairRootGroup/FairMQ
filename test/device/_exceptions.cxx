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

void RunExceptionIn(const string& state, const string& control, const string& input = "")
{
    size_t session{fair::mq::tools::UuidHash()};

    execute_result result{"", 100};
    thread device_thread([&]() {
        stringstream cmd;
        cmd << runTestDevice
            << " --id exceptions_" << state << "_"
            << " --control " << control
            << " --session " << session
            << " --color false";
        result = execute(cmd.str(), "[EXCEPTION IN " + state + "]", input);
    });

    device_thread.join();

    ASSERT_NE(string::npos, result.console_out.find("exception in " + state + "()"));

    exit(result.exit_code);
}

TEST(Exceptions, static_InInit)
{
    EXPECT_EXIT(RunExceptionIn("Init", "static"), ::testing::ExitedWithCode(1), "");
}
TEST(Exceptions, static_InBind)
{
    EXPECT_EXIT(RunExceptionIn("Bind", "static"), ::testing::ExitedWithCode(1), "");
}
TEST(Exceptions, static_InConnect)
{
    EXPECT_EXIT(RunExceptionIn("Connect", "static"), ::testing::ExitedWithCode(1), "");
}
TEST(Exceptions, static_InInitTask)
{
    EXPECT_EXIT(RunExceptionIn("InitTask", "static"), ::testing::ExitedWithCode(1), "");
}
TEST(Exceptions, static_InPreRun)
{
    EXPECT_EXIT(RunExceptionIn("PreRun", "static"), ::testing::ExitedWithCode(1), "");
}
TEST(Exceptions, static_InRun)
{
    EXPECT_EXIT(RunExceptionIn("Run", "static"), ::testing::ExitedWithCode(1), "");
}
TEST(Exceptions, static_InPostRun)
{
    EXPECT_EXIT(RunExceptionIn("PostRun", "static"), ::testing::ExitedWithCode(1), "");
}
TEST(Exceptions, static_InResetTask)
{
    EXPECT_EXIT(RunExceptionIn("ResetTask", "static"), ::testing::ExitedWithCode(1), "");
}
TEST(Exceptions, static_InReset)
{
    EXPECT_EXIT(RunExceptionIn("Reset", "static"), ::testing::ExitedWithCode(1), "");
}
TEST(Exceptions, interactive_InInit)
{
    EXPECT_EXIT(RunExceptionIn("Init", "interactive"), ::testing::ExitedWithCode(1), "");
}
TEST(Exceptions, interactive_InBind)
{
    EXPECT_EXIT(RunExceptionIn("Bind", "interactive"), ::testing::ExitedWithCode(1), "");
}
TEST(Exceptions, interactive_InConnect)
{
    EXPECT_EXIT(RunExceptionIn("Connect", "interactive"), ::testing::ExitedWithCode(1), "");
}
TEST(Exceptions, interactive_InInitTask)
{
    EXPECT_EXIT(RunExceptionIn("InitTask", "interactive"), ::testing::ExitedWithCode(1), "");
}
TEST(Exceptions, interactive_InPreRun)
{
    EXPECT_EXIT(RunExceptionIn("PreRun", "interactive"), ::testing::ExitedWithCode(1), "");
}
TEST(Exceptions, interactive_InRun)
{
    EXPECT_EXIT(RunExceptionIn("Run", "interactive"), ::testing::ExitedWithCode(1), "");
}
TEST(Exceptions, interactive_InPostRun)
{
    EXPECT_EXIT(RunExceptionIn("PostRun", "interactive"), ::testing::ExitedWithCode(1), "");
}
TEST(Exceptions, interactive_InResetTask)
{
    EXPECT_EXIT(RunExceptionIn("ResetTask", "interactive", "q"), ::testing::ExitedWithCode(1), "");
}
TEST(Exceptions, interactive_InReset)
{
    EXPECT_EXIT(RunExceptionIn("Reset", "interactive", "q"), ::testing::ExitedWithCode(1), "");
}

} // namespace
