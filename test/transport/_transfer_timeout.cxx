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

#include <gtest/gtest.h>

#include <chrono>
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
    stringstream cmd;
    cmd << runTestDevice << " --id transfer_timeout_" << transport << " --control static --transport " << transport
        << " --session " << session << " --color false --mq-config \"" << mqConfig << "\"";
    auto res = execute(cmd.str());

    cerr << res.console_out;

    exit(res.exit_code);
}

void InterruptTransfer(const string& transport, const string& _address)
{
    size_t session{UuidHash()};
    std::string address(ToString(_address, "_", transport));

    fair::mq::ProgOptions config;
    config.SetProperty<string>("session", to_string(session));

    auto factory = TransportFactory::CreateTransportFactory(transport, Uuid(), &config);

    Channel pull{"Pull", "pull", factory};
    pull.Bind(address);

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
    InterruptTransfer("zeromq", "ipc://test_interrupt_transfer");
}

TEST(InterruptTransfer, shmem)
{
    InterruptTransfer("shmem", "ipc://test_interrupt_transfer");
}

} // namespace
