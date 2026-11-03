/********************************************************************************
 *    Copyright (C) 2026 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <fairmq/Channel.h>
#include <fairmq/ProgOptions.h>
#include <fairmq/TransportFactory.h>
#include <fairmq/tools/Unique.h>

#include <helper/Transport.h>
#include <helper/TransportParam.h>

#include <gtest/gtest.h>

#include <set>
#include <string>

namespace
{

using namespace std;
using namespace fair::mq;

auto Port(string const& endpoint) -> int
{
    return stoi(endpoint.substr(endpoint.rfind(':') + 1));
}

auto ReceiveString(Channel& channel) -> string
{
    auto msg(channel.NewMessage());
    if (channel.Receive(msg, 5000) < 0) {
        return "<receive failed>";
    }
    return {static_cast<char const*>(msg->GetData()), msg->GetSize()};
}

using Tcp = test::TransportTest;

// Port 1 is the FairMQ convention for "any port": without the rights to bind it,
// or when it is in use, BindEndpoint() picks a random port in the port range.
TEST_P(Tcp, AutoBind)
{
    ProgOptions config;
    test::Configure(config, tools::Uuid(), GetParam().expandedShmMetadata);
    auto factory = TransportFactory::CreateTransportFactory(GetParam().transport, tools::Uuid(), &config);

    Channel push1("push1", "push", factory);
    Channel push2("push2", "push", factory);
    Channel pull("pull", "pull", factory);
    push1.UpdateAutoBind(true);
    push2.UpdateAutoBind(true);

    string endpoint1 = "tcp://127.0.0.1:1";
    ASSERT_TRUE(push1.BindEndpoint(endpoint1));
    // The same endpoint is now in use: push2 must get another port.
    string endpoint2 = endpoint1;
    ASSERT_TRUE(push2.BindEndpoint(endpoint2));
    EXPECT_NE(endpoint2, endpoint1);
    EXPECT_GE(Port(endpoint2), push2.GetPortRangeMin());
    EXPECT_LE(Port(endpoint2), push2.GetPortRangeMax());

    ASSERT_TRUE(pull.Connect(endpoint1));
    ASSERT_TRUE(pull.Connect(endpoint2));
    auto msg1(push1.NewSimpleMessage("1"));
    auto msg2(push2.NewSimpleMessage("2"));
    ASSERT_EQ(push1.Send(msg1, 5000), 1);
    ASSERT_EQ(push2.Send(msg2, 5000), 1);
    EXPECT_EQ((set<string>{ReceiveString(pull), ReceiveString(pull)}), (set<string>{"1", "2"}));
}

TEST_P(Tcp, NoAutoBind)
{
    ProgOptions config;
    test::Configure(config, tools::Uuid(), GetParam().expandedShmMetadata);
    auto factory = TransportFactory::CreateTransportFactory(GetParam().transport, tools::Uuid(), &config);

    Channel first("first", "push", factory);
    string endpoint = "tcp://127.0.0.1:1";
    ASSERT_TRUE(first.BindEndpoint(endpoint));

    Channel second("second", "push", factory);
    second.UpdateAutoBind(false);
    string taken = endpoint;
    EXPECT_FALSE(second.BindEndpoint(taken));
    EXPECT_EQ(taken, endpoint);
}

TEST_P(Tcp, Pair)
{
    ProgOptions config;
    test::Configure(config, tools::Uuid(), GetParam().expandedShmMetadata);
    auto factory = TransportFactory::CreateTransportFactory(GetParam().transport, tools::Uuid(), &config);

    Channel left("left", "pair", factory);
    Channel right("right", "pair", factory);
    string endpoint = "tcp://127.0.0.1:1";
    ASSERT_TRUE(left.BindEndpoint(endpoint));
    ASSERT_TRUE(right.Connect(endpoint));

    auto ping(left.NewSimpleMessage("ping"));
    ASSERT_EQ(left.Send(ping, 5000), 4);
    EXPECT_EQ(ReceiveString(right), "ping");
    auto pong(right.NewSimpleMessage("pong"));
    ASSERT_EQ(right.Send(pong, 5000), 4);
    EXPECT_EQ(ReceiveString(left), "pong");
}

INSTANTIATE_TEST_SUITE_P(, Tcp, test::AllTransports(), test::TransportParamName());

} // namespace
