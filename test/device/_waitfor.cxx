/********************************************************************************
 * Copyright (C) 2014-2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "runner.h"

#include <gtest/gtest.h>
#include <fairmq/Tools.h>

#include <signal.h> // kill

#include <string>
#include <thread>
#include <iostream>

namespace
{

using namespace std;
using namespace fair::mq::test;
using namespace fair::mq::tools;

void RunWaitFor(const string& state, const string& control)
{
    execute_result result;
    thread device_thread([&] {
        stringstream cmd;
        cmd << runTestDevice
            << " --id waitfor_" << state
            << " --control " << control
            << " --session " << UuidHash()
            << " --severity debug"
            << " --color false";
        result = execute(cmd.str(), "[WaitFor]", "", SIGINT);
    });

    device_thread.join();

    ASSERT_NE(string::npos, result.console_out.find("Sleeping Done. Interrupted."));

    exit(result.exit_code);
}

TEST(WaitFor, static_InPreRun)
{
    EXPECT_EXIT(RunWaitFor("PreRun", "static"), ::testing::ExitedWithCode(0), "");
}
TEST(WaitFor, static_InRun)
{
    EXPECT_EXIT(RunWaitFor("Run", "static"), ::testing::ExitedWithCode(0), "");
}
TEST(WaitFor, static_InPostRun)
{
    EXPECT_EXIT(RunWaitFor("PostRun", "static"), ::testing::ExitedWithCode(0), "");
}
TEST(WaitFor, interactive_InPreRun)
{
    EXPECT_EXIT(RunWaitFor("PreRun", "interactive"), ::testing::ExitedWithCode(0), "");
}
TEST(WaitFor, interactive_InRun)
{
    EXPECT_EXIT(RunWaitFor("Run", "interactive"), ::testing::ExitedWithCode(0), "");
}
TEST(WaitFor, interactive_InPostRun)
{
    EXPECT_EXIT(RunWaitFor("PostRun", "interactive"), ::testing::ExitedWithCode(0), "");
}

} // namespace
