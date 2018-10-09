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

void RunExceptionIn(const std::string& state)
{
    size_t session{fair::mq::tools::UuidHash()};

    execute_result result{"", 100};
    thread device_thread([&]() {
        stringstream cmd;
        cmd << runTestDevice
            << " --id exceptions_" << state << "_"
            << " --control static"
            << " --session " << session
            << " --color false";
        result = execute(cmd.str(), "[EXCEPTION IN " + state + "]");
    });

    device_thread.join();

    ASSERT_NE(std::string::npos, result.console_out.find("exception in " + state + "()"));

    exit(result.exit_code);
}

TEST(Exceptions, InInit) { EXPECT_EXIT(RunExceptionIn("Init"), ::testing::ExitedWithCode(1), ""); }
TEST(Exceptions, InInitTask)
{
    EXPECT_EXIT(RunExceptionIn("InitTask"), ::testing::ExitedWithCode(1), "");
}
TEST(Exceptions, InPreRun)
{
    EXPECT_EXIT(RunExceptionIn("PreRun"), ::testing::ExitedWithCode(1), "");
}
TEST(Exceptions, InRun) { EXPECT_EXIT(RunExceptionIn("Run"), ::testing::ExitedWithCode(1), ""); }
TEST(Exceptions, InPostRun)
{
    EXPECT_EXIT(RunExceptionIn("PostRun"), ::testing::ExitedWithCode(1), "");
}
TEST(Exceptions, InResetTask)
{
    EXPECT_EXIT(RunExceptionIn("ResetTask"), ::testing::ExitedWithCode(1), "");
}
TEST(Exceptions, InReset)
{
    EXPECT_EXIT(RunExceptionIn("Reset"), ::testing::ExitedWithCode(1), "");
}

} // namespace
