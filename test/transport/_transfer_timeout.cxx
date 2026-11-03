/********************************************************************************
 *    Copyright (C) 2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "runner.h"

#include <fairmq/Channel.h>
#include <fairmq/TransportFactory.h>
#include <fairmq/ProgOptions.h>
#include <fairmq/tools/Process.h>
#include <fairmq/tools/Strings.h>
#include <fairmq/tools/Unique.h>

#include <fairlogger/Logger.h>

#include <helper/Transport.h>
#include <gtest/gtest.h>

#include <chrono>
#include <cstdio> // std::remove
#include <sstream> // std::stringstream
#include <thread>

namespace
{

using namespace std;
using namespace fair::mq;
using namespace fair::mq::test;
using namespace fair::mq::tools;

void delayedInterruptor(TransportFactory& transport)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    transport.Interrupt();
}

auto RunTransferTimeout(string transport) -> void
{
    size_t session{UuidHash()};
    string dataInIpcFile("/tmp/fmq_" + to_string(session) + "_datain_" + transport);
    string dataOutIpcFile("/tmp/fmq_" + to_string(session) + "_dataout_" + transport);
    string dataInAddress("ipc://" + dataInIpcFile);
    string dataOutAddress("ipc://" + dataOutIpcFile);

    stringstream cmd;
    cmd << runTestDevice
        << " --id transfer_timeout_" << transport
        << " --control static"
        << " --shm-segment-size 100000000"
        << " --severity debug"
        << " --shm-monitor true"
        << " --transport " << transport
        << " --session " << session
        << " --color false"
        << " --channel-config name=data-in,type=pull,method=bind,address=" << dataInAddress
        << "                  name=data-out,type=push,method=bind,address=" << dataOutAddress;
    auto res = execute(cmd.str());

    cerr << res.console_out;

    std::remove(dataInIpcFile.c_str());
    std::remove(dataOutIpcFile.c_str());

    exit(res.exit_code);
}

void InterruptTransfer(const string& transport, const string& name)
{
    size_t session{UuidHash()};
    test::IpcAddress const address(ToString(name, "_", transport));

    fair::mq::ProgOptions config;
    test::Configure(config, to_string(session));

    auto factory = TransportFactory::CreateTransportFactory(transport, Uuid(), &config);

    Channel pull{"Pull", "pull", factory};
    ASSERT_TRUE(pull.Bind(address));

    MessagePtr msg(pull.NewMessage());

    auto t = thread(delayedInterruptor, ref(*factory));

    auto result = pull.Receive(msg);
    t.join();
    ASSERT_EQ(result, static_cast<int>(fair::mq::TransferCode::interrupted));
}

TEST(TransferTimeout, zeromq)
{
    EXPECT_EXIT(RunTransferTimeout("zeromq"), ::testing::ExitedWithCode(0), "Transfer timeout test successfull");
}

TEST(TransferTimeout, shmem)
{
    EXPECT_EXIT(RunTransferTimeout("shmem"), ::testing::ExitedWithCode(0), "Transfer timeout test successfull");
}

TEST(InterruptTransfer, zeromq)
{
    InterruptTransfer("zeromq", "interrupt_transfer");
}

TEST(InterruptTransfer, shmem)
{
    InterruptTransfer("shmem", "interrupt_transfer");
}

} // namespace
