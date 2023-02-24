/********************************************************************************
 *    Copyright (C) 2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "runner.h"
#include <fairmq/tools/Unique.h>
#include <fairmq/tools/Process.h>
#include <fairmq/tools/Strings.h>
#include <gtest/gtest.h>
#include <cstdio> // std::remove
#include <sstream> // std::stringstream
#include <thread>

namespace
{

using namespace std;
using namespace fair::mq::test;
using namespace fair::mq::tools;

auto RunPushPull(string transport) -> void
{
    size_t session(fair::mq::tools::UuidHash());
    string ipcFile("/tmp/fmq_" + to_string(session) + "_data_" + transport);
    string address("ipc://" + ipcFile);

    auto push = execute_result{"", 100};
    thread push_thread([&]() {
        stringstream cmd;
        cmd << runTestDevice
            << " --id push_" << transport
            << " --control static"
            << " --shm-monitor true"
            << " --shm-segment-size 100000000"
            << " --session " << session
            << " --color false"
            << " --channel-config name=data,type=push,method=bind,address=" << address;
        push = execute(cmd.str(), "[PUSH]");
    });

    auto pull = execute_result{"", 100};
    thread pull_thread([&]() {
        stringstream cmd;
        cmd << runTestDevice
            << " --id pull_" << transport
            << " --control static"
            << " --shm-monitor true"
            << " --shm-segment-size 100000000"
            << " --session " << session
            << " --color false"
            << " --channel-config name=data,type=pull,method=connect,address=" << address;
        pull = execute(cmd.str(), "[PULL]");
    });

    push_thread.join();
    pull_thread.join();
    cerr << push.console_out << pull.console_out;

    std::remove(ipcFile.c_str());

    exit(push.exit_code + pull.exit_code);
}

TEST(PushPull, SingleMsg_MP_ipc_zeromq)
{
    EXPECT_EXIT(RunPushPull("zeromq"), ::testing::ExitedWithCode(0), "PUSH-PULL test successfull");
}

TEST(PushPull, SingleMsg_MP_ipc_shmem)
{
    EXPECT_EXIT(RunPushPull("shmem"), ::testing::ExitedWithCode(0), "PUSH-PULL test successfull");
}

} // namespace
