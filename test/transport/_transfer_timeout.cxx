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
#include <fairmq/tools/Unique.h>

#include <helper/Devices.h>
#include <helper/Transport.h>
#include <helper/TransportParam.h>

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

using TransferTimeout = TransportTest;

TEST_P(TransferTimeout, Device)
{
    auto const& transport = GetParam().transport;
    IpcAddress const dataIn("transfer_timeout_in");
    IpcAddress const dataOut("transfer_timeout_out");

    auto const results = RunDevices({
        {"transfer_timeout_" + transport,
         "name=data-in,type=pull,method=bind,address=" + dataIn.str()
         + " name=data-out,type=push,method=bind,address=" + dataOut.str()},
    }, RunOptions(transport, GetParam().DeviceArgs() + " --severity debug"));

    EXPECT_TRUE(Succeeded(results.at(0), transport));
}

using InterruptTransfer = TransportTest;

TEST_P(InterruptTransfer, Receive)
{
    auto const& transport = GetParam().transport;
    test::IpcAddress const address("interrupt_transfer_" + transport);

    fair::mq::ProgOptions config;
    test::Configure(config, Uuid(), GetParam().expandedShmMetadata);

    auto factory = TransportFactory::CreateTransportFactory(transport, Uuid(), &config);

    Channel pull{"Pull", "pull", factory};
    ASSERT_TRUE(pull.Bind(address));

    MessagePtr msg(pull.NewMessage());

    auto t = thread(delayedInterruptor, ref(*factory));

    auto result = pull.Receive(msg);
    t.join();
    ASSERT_EQ(result, static_cast<int>(fair::mq::TransferCode::interrupted));
}

INSTANTIATE_TEST_SUITE_P(, TransferTimeout, AllTransports(), TransportParamName());
INSTANTIATE_TEST_SUITE_P(, InterruptTransfer, AllTransports(), TransportParamName());

} // namespace
