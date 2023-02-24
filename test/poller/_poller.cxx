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

auto RunPoller(string transport, int pollType) -> void
{
    size_t session{UuidHash()};
    string data1IpcFile("/tmp/fmq_" + to_string(session) + "_data1_" + transport);
    string data2IpcFile("/tmp/fmq_" + to_string(session) + "_data2_" + transport);
    string data1Address("ipc://" + data1IpcFile);
    string data2Address("ipc://" + data2IpcFile);

    auto pollout = execute_result{"", 0};
    thread poll_out_thread([&]() {
        stringstream cmd;
        cmd << runTestDevice
            << " --id pollout_"<< transport
            << " --control static"
            << " --color false"
            << " --shm-monitor true"
            << " --session " << session
            << " --channel-config name=data1,type=push,method=bind,address=" << data1Address
            << "                  name=data2,type=push,method=bind,address=" << data2Address;
        pollout = execute(cmd.str(), "[POLLOUT]");
    });

    auto pollin = execute_result{"", 0};
    thread poll_in_thread([&]() {
        stringstream cmd;
        cmd << runTestDevice
            << " --id pollin_" << transport
            << " --control static"
            << " --color false"
            << " --shm-monitor true"
            << " --session " << session
            << " --poll-type " << pollType
            << " --channel-config name=data1,type=pull,method=connect,address=" << data1Address
            << "                  name=data2,type=pull,method=connect,address=" << data2Address;
        pollin = execute(cmd.str(), "[POLLIN]");
    });

    poll_out_thread.join();
    poll_in_thread.join();
    cerr << pollout.console_out << pollin.console_out;

    std::remove(data1IpcFile.c_str());
    std::remove(data2IpcFile.c_str());

    exit(pollout.exit_code + pollin.exit_code);
}

TEST(Subchannel, zeromq)
{
    EXPECT_EXIT(RunPoller("zeromq", 0), ::testing::ExitedWithCode(0), "POLL test successfull");
}

TEST(Subchannel, shmem)
{
    EXPECT_EXIT(RunPoller("shmem", 0), ::testing::ExitedWithCode(0), "POLL test successfull");
}

TEST(Channel, zeromq)
{
    EXPECT_EXIT(RunPoller("zeromq", 1), ::testing::ExitedWithCode(0), "POLL test successfull");
}

TEST(Channel, shmem)
{
    EXPECT_EXIT(RunPoller("shmem", 1), ::testing::ExitedWithCode(0), "POLL test successfull");
}

} // namespace
