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

auto RunPubSub(string transport) -> void
{
    size_t session{fair::mq::tools::UuidHash()};
    string dataIpcFile("/tmp/fmq_" + to_string(session) + "_data_" + transport);
    string ctrlIpcFile("/tmp/fmq_" + to_string(session) + "_ctrl_" + transport);
    string dataAddress("ipc://" + dataIpcFile);
    string ctrlAddress("ipc://" + ctrlIpcFile);

    auto pub = execute_result{"", 0};
    thread pub_thread([&]() {
        stringstream cmd;
        cmd << runTestDevice
            << " --id pub_" << transport
            << " --control static"
            << " --session " << session
            << " --color false"
            << " --channel-config name=data,type=pub,method=bind,address=" << dataAddress
            << "                  name=control,type=pull,method=bind,address=" << ctrlAddress;
        pub = execute(cmd.str(), "[PUB]");
    });

    auto sub1 = execute_result{"", 0};
    thread sub1_thread([&]() {
        stringstream cmd;
        cmd << runTestDevice
            << " --id sub_1" << transport
            << " --control static"
            << " --session " << session
            << " --color false"
            << " --channel-config name=data,type=sub,method=connect,address=" << dataAddress
            << "                  name=control,type=push,method=connect,address=" << ctrlAddress;
        sub1 = execute(cmd.str(), "[SUB1]");
    });

    auto sub2 = execute_result{"", 0};
    thread sub2_thread([&]() {
        stringstream cmd;
        cmd << runTestDevice
            << " --id sub_2" << transport
            << " --control static"
            << " --session " << session
            << " --color false"
            << " --channel-config name=data,type=sub,method=connect,address=" << dataAddress
            << "                  name=control,type=push,method=connect,address=" << ctrlAddress;
        sub2 = execute(cmd.str(), "[SUB2]");
    });

    pub_thread.join();
    sub1_thread.join();
    sub2_thread.join();
    cerr << pub.console_out << sub1.console_out << sub2.console_out << endl;

    std::remove(dataIpcFile.c_str());
    std::remove(ctrlIpcFile.c_str());

    exit(pub.exit_code + sub1.exit_code + sub2.exit_code);
}

TEST(PubSub, zeromq)
{
    EXPECT_EXIT(RunPubSub("zeromq"), ::testing::ExitedWithCode(0), "PUB-SUB test successfull");
}

} // namespace
