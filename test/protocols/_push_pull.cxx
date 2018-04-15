/********************************************************************************
 *    Copyright (C) 2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "runner.h"
#include <fairmq/Tools.h>
#include <gtest/gtest.h>
#include <sstream> // std::stringstream
#include <thread>

namespace
{

using namespace std;
using namespace fair::mq::test;
using namespace fair::mq::tools;

auto RunPushPull(string transport) -> void
{
    size_t session{fair::mq::tools::UuidHash()};

    auto push = execute_result{"", 100};
    thread push_thread([&]() {
        stringstream cmd;
        cmd << runTestDevice << " --id push_" << transport << " --control static "
        << "--session " << session << " --color false --mq-config \"" << mqConfig << "\"";
        push = execute(cmd.str(), "[PUSH]");
    });

    auto pull = execute_result{"", 100};
    thread pull_thread([&]() {
        stringstream cmd;
        cmd << runTestDevice << " --id pull_" << transport << " --control static "
        << "--session " << session << " --color false --mq-config \"" << mqConfig << "\"";
        pull = execute(cmd.str(), "[PULL]");
    });

    push_thread.join();
    pull_thread.join();
    cerr << push.console_out << pull.console_out;

    exit(push.exit_code + pull.exit_code);
}

TEST(PushPull, MP_ZeroMQ__tcp____SingleMsg)
{
    EXPECT_EXIT(RunPushPull("zeromq"), ::testing::ExitedWithCode(0), "PUSH-PULL test successfull");
}

TEST(PushPull, MP_ShMem___tcp____SingleMsg)
{
    EXPECT_EXIT(RunPushPull("shmem"), ::testing::ExitedWithCode(0), "PUSH-PULL test successfull");
}

#ifdef BUILD_NANOMSG_TRANSPORT
TEST(PushPull, MP_Nanomsg_tcp____SingleMsg)
{
    EXPECT_EXIT(RunPushPull("nanomsg"), ::testing::ExitedWithCode(0), "PUSH-PULL test successfull");
}
#endif /* BUILD_NANOMSG_TRANSPORT */

} // namespace
