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

auto RunReqRep(string transport, const string& extraDeviceCmdArgs) -> void
{
    size_t session{fair::mq::tools::UuidHash()};
    string ipcFile("/tmp/fmq_" + to_string(session) + "_data_" + transport);
    string address("ipc://" + ipcFile);

    auto rep = execute_result{"", 0};
    thread rep_thread([&]() {
        stringstream cmd;
        cmd << runTestDevice
            << " --id rep_" << transport
            << " --control static"
            << " --shm-monitor true"
            << " --shm-segment-size 100000000"
            << " --session " << session
            << " --color false"
            << extraDeviceCmdArgs
            << " --channel-config name=data,type=rep,method=bind,address=" << address;
        rep = execute(cmd.str(), "[REP]");
    });

    auto req1 = execute_result{"", 0};
    thread req1_thread([&]() {
        stringstream cmd;
        cmd << runTestDevice
            << " --id req_1" << transport
            << " --control static"
            << " --shm-monitor true"
            << " --shm-segment-size 100000000"
            << " --session " << session
            << " --color false"
            << extraDeviceCmdArgs
            << " --channel-config name=data,type=req,method=connect,address=" << address;
        req1 = execute(cmd.str(), "[REQ1]");
    });

    auto req2 = execute_result{"", 0};
    thread req2_thread([&]() {
        stringstream cmd;
        cmd << runTestDevice
            << " --id req_2" << transport
            << " --control static"
            << " --shm-monitor true"
            << " --shm-segment-size 100000000"
            << " --session " << session
            << " --color false"
            << extraDeviceCmdArgs
            << " --channel-config name=data,type=req,method=connect,address=" << address;
        req2 = execute(cmd.str(), "[REQ2]");
    });

    rep_thread.join();
    req1_thread.join();
    req2_thread.join();
    cerr << req1.console_out << req2.console_out << rep.console_out;

    std::remove(ipcFile.c_str());

    exit(req1.exit_code + req2.exit_code + rep.exit_code);
}

TEST(ReqRep, zeromq)
{
    EXPECT_EXIT(RunReqRep("zeromq", ""), ::testing::ExitedWithCode(0), "REQ-REP test successfull");
}

TEST(ReqRep, shmem)
{
    EXPECT_EXIT(RunReqRep("shmem", ""), ::testing::ExitedWithCode(0), "REQ-REP test successfull");
}

TEST(ReqRep, shmem_expanded_metadata)
{
    EXPECT_EXIT(RunReqRep("shmem", " --shm-metadata-msg-size 2048"), ::testing::ExitedWithCode(0), "REQ-REP test successfull");
}

} // namespace
