/********************************************************************************
 *    Copyright (C) 2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
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

auto RunPair(string transport) -> void
{
    size_t session{fair::mq::tools::UuidHash()};
    string ipcFile("/tmp/fmq_" + to_string(session) + "_data_" + transport);
    string address("ipc://" + ipcFile);

    // ofi does not run with ipc://
    if (transport == "ofi") {
        address = "tcp://127.0.0.1:5957";
    }

    auto pairleft = execute_result{"", 100};
    thread pairleft_thread([&]() {
        stringstream cmd;
        cmd << runTestDevice
            << " --id pairleft_" << transport
            << " --control static"
            << " --shm-monitor true"
            << " --shm-segment-size 100000000"
            << " --session " << session
            << " --color false"
            << " --channel-config name=data,type=pair,method=bind,address=" << address;
        pairleft = execute(cmd.str(), "[PAIR L]");
    });

    auto pairright = execute_result{"", 100};
    thread pairright_thread([&]() {
        stringstream cmd;
        cmd << runTestDevice
            << " --id pairright_" << transport
            << " --control static"
            << " --shm-monitor true"
            << " --shm-segment-size 100000000"
            << " --session " << session
            << " --color false"
            << " --channel-config name=data,type=pair,method=connect,address=" << address;
        pairright = execute(cmd.str(), "[PAIR R]");
    });

    pairleft_thread.join();
    pairright_thread.join();
    cerr << pairleft.console_out << pairright.console_out;

    std::remove(ipcFile.c_str());

    exit(pairleft.exit_code + pairright.exit_code);
}

TEST(Pair, SingleMsg_MP_tcp_zeromq)
{
    EXPECT_EXIT(RunPair("zeromq"), ::testing::ExitedWithCode(0), "PAIR test successfull");
}

TEST(Pair, SingleMsg_MP_tcp_shmem)
{
    EXPECT_EXIT(RunPair("shmem"), ::testing::ExitedWithCode(0), "PAIR test successfull");
}

#ifdef BUILD_OFI_TRANSPORT
TEST(Pair, SingleMsg_MP_tcp_ofi)
{
    EXPECT_EXIT(RunPair("ofi"), ::testing::ExitedWithCode(0), "PAIR test successfull");
}
#endif /* BUILD_OFI_TRANSPORT */

} // namespace
