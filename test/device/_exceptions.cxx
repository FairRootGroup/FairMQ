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

TEST(Exceptions, InInit_______static)
{
    EXPECT_EXIT(RunExceptionIn("Init"), ::testing::ExitedWithCode(1), "");
}
TEST(Exceptions, InInitTask___static)
{
    EXPECT_EXIT(RunExceptionIn("InitTask"), ::testing::ExitedWithCode(1), "");
}
TEST(Exceptions, InPreRun_____static)
{
    EXPECT_EXIT(RunExceptionIn("PreRun"), ::testing::ExitedWithCode(1), "");
}
TEST(Exceptions, InRun________static)
{
    EXPECT_EXIT(RunExceptionIn("Run"), ::testing::ExitedWithCode(1), "");
}
TEST(Exceptions, InPostRun____static)
{
    EXPECT_EXIT(RunExceptionIn("PostRun"), ::testing::ExitedWithCode(1), "");
}
TEST(Exceptions, InResetTask__static)
{
    EXPECT_EXIT(RunExceptionIn("ResetTask"), ::testing::ExitedWithCode(1), "");
}
TEST(Exceptions, InReset______static)
{
    EXPECT_EXIT(RunExceptionIn("Reset"), ::testing::ExitedWithCode(1), "");
}
TEST(Exceptions, InInit_______interactive)
{
    EXPECT_EXIT(RunExceptionIn("Init", "q"), ::testing::ExitedWithCode(1), "");
}
TEST(Exceptions, InInitTask___interactive)
{
    EXPECT_EXIT(RunExceptionIn("InitTask", "q"), ::testing::ExitedWithCode(1), "");
}
TEST(Exceptions, InPreRun_____interactive)
{
    EXPECT_EXIT(RunExceptionIn("PreRun", "q"), ::testing::ExitedWithCode(1), "");
}
TEST(Exceptions, InRun________interactive)
{
    EXPECT_EXIT(RunExceptionIn("Run", "q"), ::testing::ExitedWithCode(1), "");
}
TEST(Exceptions, InPostRun____interactive)
{
    EXPECT_EXIT(RunExceptionIn("PostRun", "q"), ::testing::ExitedWithCode(1), "");
}
TEST(Exceptions, InResetTask__interactive)
{
    EXPECT_EXIT(RunExceptionIn("ResetTask", "q"), ::testing::ExitedWithCode(1), "");
}
TEST(Exceptions, InReset______interactive)
{
    EXPECT_EXIT(RunExceptionIn("Reset", "q"), ::testing::ExitedWithCode(1), "");
}
TEST(Exceptions, InInit_______interactive_invalid)
{
    EXPECT_EXIT(RunExceptionIn("Init", "_"), ::testing::ExitedWithCode(1), "");
}
TEST(Exceptions, InInitTask___interactive_invalid)
{
    EXPECT_EXIT(RunExceptionIn("InitTask", "_"), ::testing::ExitedWithCode(1), "");
}
TEST(Exceptions, InPreRun_____interactive_invalid)
{
    EXPECT_EXIT(RunExceptionIn("PreRun", "_"), ::testing::ExitedWithCode(1), "");
}
TEST(Exceptions, InRun________interactive_invalid)
{
    EXPECT_EXIT(RunExceptionIn("Run", "_"), ::testing::ExitedWithCode(1), "");
}
TEST(Exceptions, InPostRun____interactive_invalid)
{
    EXPECT_EXIT(RunExceptionIn("PostRun", "_"), ::testing::ExitedWithCode(1), "");
}

} // namespace
