/********************************************************************************
 *    Copyright (C) 2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "runner.h"
#include <gtest/gtest.h>
#include <sstream> // std::stringstream
#include <thread>

namespace
{

using namespace std;
using namespace fair::mq::test;

auto RunPushPull(string transport) -> void
{
    auto push = execute_result{"", 100};
    thread push_thread([&]() {
        stringstream cmd;
        cmd << runTestDevice << " --id push_" << transport << " --control static --verbosity DEBUG "
            << "--log-color false --mq-config \"" << mqConfig << "\"";
        push = execute(cmd.str(), "[PUSH]");
    });

    auto pull = execute_result{"", 100};
    thread pull_thread([&]() {
        stringstream cmd;
        cmd << runTestDevice << " --id pull_" << transport << " --control static --verbosity DEBUG "
            << "--log-color false --mq-config \"" << mqConfig << "\"";
        pull = execute(cmd.str(), "[PULL]");
    });

    push_thread.join();
    pull_thread.join();
    cerr << push.error_out << pull.error_out;

    exit(push.exit_code + pull.exit_code);
}

TEST(PushPull, ZeroMQ)
{
    EXPECT_EXIT(RunPushPull("zeromq"), ::testing::ExitedWithCode(0), "PUSH-PULL test successfull");
}

TEST(PushPull, ShMem)
{
    EXPECT_EXIT(RunPushPull("shmem"), ::testing::ExitedWithCode(0), "PUSH-PULL test successfull");
}

#ifdef NANOMSG_FOUND
TEST(PushPull, Nanomsg)
{
    EXPECT_EXIT(RunPushPull("nanomsg"), ::testing::ExitedWithCode(0), "PUSH-PULL test successfull");
}
#endif /* NANOMSG_FOUND */

} // namespace
