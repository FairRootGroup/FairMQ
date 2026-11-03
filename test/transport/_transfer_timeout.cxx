/********************************************************************************
 *    Copyright (C) 2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <fairmq/Channel.h>
#include <fairmq/TransportFactory.h>
#include <fairmq/ProgOptions.h>
#include <fairmq/tools/Strings.h>
#include <fairmq/tools/Unique.h>

#include <helper/Devices.h>
#include <helper/Transport.h>

#include <gtest/gtest.h>

#include <chrono>
#include <string>
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

auto RunTransferTimeout(string const& transport) -> void
{
    IpcAddress const dataIn("transfer_timeout_in");
    IpcAddress const dataOut("transfer_timeout_out");

    auto const results = RunDevices({
        {"transfer_timeout_" + transport,
         "name=data-in,type=pull,method=bind,address=" + dataIn.str()
         + " name=data-out,type=push,method=bind,address=" + dataOut.str()},
    }, RunOptions(transport, "--severity debug"));

    EXPECT_TRUE(Succeeded(results.at(0), transport));
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
    RunTransferTimeout("zeromq");
}

TEST(TransferTimeout, shmem)
{
    RunTransferTimeout("shmem");
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
