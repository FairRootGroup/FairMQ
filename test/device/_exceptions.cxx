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

void RunExceptionIn(const std::string& state, const std::string& input = "")
{
    size_t session{fair::mq::tools::UuidHash()};

    execute_result result{"", 100};
    thread device_thread([&]() {
        stringstream cmd;
        cmd << runTestDevice
            << " --id exceptions_" << state << "_"
            << " --control " << ((input == "") ? "static" : "interactive")
            << " --session " << session
            << " --color false";
        result = execute(cmd.str(), "[EXCEPTION IN " + state + "]", input);
    });

    device_thread.join();

    ASSERT_NE(std::string::npos, result.console_out.find("exception in " + state + "()"));

    exit(result.exit_code);
}

TEST(Exceptions, static_InInit)
{
    EXPECT_EXIT(RunExceptionIn("Init"), ::testing::ExitedWithCode(1), "");
}
TEST(Exceptions, static_InBind)
{
    EXPECT_EXIT(RunExceptionIn("Bind"), ::testing::ExitedWithCode(1), "");
}
TEST(Exceptions, static_InConnect)
{
    EXPECT_EXIT(RunExceptionIn("Connect"), ::testing::ExitedWithCode(1), "");
}
TEST(Exceptions, static_InInitTask)
{
    EXPECT_EXIT(RunExceptionIn("InitTask"), ::testing::ExitedWithCode(1), "");
}
TEST(Exceptions, static_InPreRun)
{
    EXPECT_EXIT(RunExceptionIn("PreRun"), ::testing::ExitedWithCode(1), "");
}
TEST(Exceptions, static_InRun)
{
    EXPECT_EXIT(RunExceptionIn("Run"), ::testing::ExitedWithCode(1), "");
}
TEST(Exceptions, static_InPostRun)
{
    EXPECT_EXIT(RunExceptionIn("PostRun"), ::testing::ExitedWithCode(1), "");
}
TEST(Exceptions, static_InResetTask)
{
    EXPECT_EXIT(RunExceptionIn("ResetTask"), ::testing::ExitedWithCode(1), "");
}
TEST(Exceptions, static_InReset)
{
    EXPECT_EXIT(RunExceptionIn("Reset"), ::testing::ExitedWithCode(1), "");
}
TEST(Exceptions, interactive_InInit)
{
    EXPECT_EXIT(RunExceptionIn("Init", "q"), ::testing::ExitedWithCode(1), "");
}
TEST(Exceptions, interactive_InBind)
{
    EXPECT_EXIT(RunExceptionIn("Bind", "q"), ::testing::ExitedWithCode(1), "");
}
TEST(Exceptions, interactive_InConnect)
{
    EXPECT_EXIT(RunExceptionIn("Connect", "q"), ::testing::ExitedWithCode(1), "");
}
TEST(Exceptions, interactive_InInitTask)
{
    EXPECT_EXIT(RunExceptionIn("InitTask", "q"), ::testing::ExitedWithCode(1), "");
}
TEST(Exceptions, interactive_InPreRun)
{
    EXPECT_EXIT(RunExceptionIn("PreRun", "q"), ::testing::ExitedWithCode(1), "");
}
TEST(Exceptions, interactive_InRun)
{
    EXPECT_EXIT(RunExceptionIn("Run", "q"), ::testing::ExitedWithCode(1), "");
}
TEST(Exceptions, interactive_InPostRun)
{
    EXPECT_EXIT(RunExceptionIn("PostRun", "q"), ::testing::ExitedWithCode(1), "");
}
TEST(Exceptions, interactive_InResetTask)
{
    EXPECT_EXIT(RunExceptionIn("ResetTask", "q"), ::testing::ExitedWithCode(1), "");
}
TEST(Exceptions, interactive_InReset)
{
    EXPECT_EXIT(RunExceptionIn("Reset", "q"), ::testing::ExitedWithCode(1), "");
}
TEST(Exceptions, interactive_invalid_InInit)
{
    EXPECT_EXIT(RunExceptionIn("Init", "_"), ::testing::ExitedWithCode(1), "");
}
TEST(Exceptions, interactive_invalid_InBind)
{
    EXPECT_EXIT(RunExceptionIn("Bind", "_"), ::testing::ExitedWithCode(1), "");
}
TEST(Exceptions, interactive_invalid_InConnect)
{
    EXPECT_EXIT(RunExceptionIn("Connect", "_"), ::testing::ExitedWithCode(1), "");
}
TEST(Exceptions, interactive_invalid_InInitTask)
{
    EXPECT_EXIT(RunExceptionIn("InitTask", "_"), ::testing::ExitedWithCode(1), "");
}
TEST(Exceptions, interactive_invalid_InPreRun)
{
    EXPECT_EXIT(RunExceptionIn("PreRun", "_"), ::testing::ExitedWithCode(1), "");
}
TEST(Exceptions, interactive_invalid_InRun)
{
    EXPECT_EXIT(RunExceptionIn("Run", "_"), ::testing::ExitedWithCode(1), "");
}
TEST(Exceptions, interactive_invalid_InPostRun)
{
    EXPECT_EXIT(RunExceptionIn("PostRun", "_"), ::testing::ExitedWithCode(1), "");
}

} // namespace
