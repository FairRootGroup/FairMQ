/********************************************************************************
 * Copyright (C) 2018-2021 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <chrono>
#include <fairmq/Channel.h>
#include <fairmq/ProgOptions.h>
#include <fairmq/Tools.h>
#include <fairmq/TransportFactory.h>
#include <gtest/gtest.h>
#include <string>
#include <thread>

namespace
{

using namespace std;
using namespace fair::mq;

TEST(Channel, Validation)
{
    Channel channel;
    ASSERT_THROW(channel.Validate(), Channel::ChannelConfigurationError);

    channel.UpdateType("pair");
    ASSERT_EQ(channel.Validate(), false);
    ASSERT_EQ(channel.IsValid(), false);

    channel.UpdateAddress("bla");
    ASSERT_THROW(channel.Validate(), Channel::ChannelConfigurationError);

    channel.UpdateMethod("connect");
    ASSERT_EQ(channel.Validate(), false);
    ASSERT_EQ(channel.IsValid(), false);

    channel.UpdateAddress("ipc://");
    ASSERT_EQ(channel.Validate(), false);
    ASSERT_EQ(channel.IsValid(), false);

    channel.UpdateAddress("verbs://");
    ASSERT_EQ(channel.Validate(), false);
    ASSERT_EQ(channel.IsValid(), false);

    channel.UpdateAddress("inproc://");
    ASSERT_EQ(channel.Validate(), false);
    ASSERT_EQ(channel.IsValid(), false);

    channel.UpdateAddress("tcp://");
    ASSERT_EQ(channel.Validate(), false);
    ASSERT_EQ(channel.IsValid(), false);

    channel.UpdateAddress("tcp://localhost:5555");
    ASSERT_EQ(channel.Validate(), true);
    ASSERT_EQ(channel.IsValid(), true);

    channel.UpdateSndBufSize(-1);
    ASSERT_THROW(channel.Validate(), Channel::ChannelConfigurationError);
    channel.UpdateSndBufSize(1000);
    ASSERT_NO_THROW(channel.Validate());

    channel.UpdateRcvBufSize(-1);
    ASSERT_THROW(channel.Validate(), Channel::ChannelConfigurationError);
    channel.UpdateRcvBufSize(1000);
    ASSERT_NO_THROW(channel.Validate());

    channel.UpdateSndKernelSize(-1);
    ASSERT_THROW(channel.Validate(), Channel::ChannelConfigurationError);
    channel.UpdateSndKernelSize(1000);
    ASSERT_NO_THROW(channel.Validate());

    channel.UpdateRcvKernelSize(-1);
    ASSERT_THROW(channel.Validate(), Channel::ChannelConfigurationError);
    channel.UpdateRcvKernelSize(1000);
    ASSERT_NO_THROW(channel.Validate());

    channel.UpdateRateLogging(-1);
    ASSERT_THROW(channel.Validate(), Channel::ChannelConfigurationError);
    channel.UpdateRateLogging(1);
    ASSERT_NO_THROW(channel.Validate());

    Channel channel2 = channel;
    ASSERT_NO_THROW(channel2.Validate());
    ASSERT_EQ(channel2.Validate(), true);
    ASSERT_EQ(channel2.IsValid(), true);
    ASSERT_EQ(channel2.Validate(), true);

    channel2.UpdateName("Kanal");
    ASSERT_EQ(channel2.GetName(), "Kanal");

    channel2.Invalidate();
    ASSERT_EQ(channel2.IsValid(), false);
    ASSERT_EQ(channel2.Validate(), true);
}

auto testConnectedPeers(std::string const& transport)
{
    using namespace std::chrono_literals;

    ProgOptions config;
    config.SetProperty<string>("session", tools::Uuid());
    string const address(tools::ToString("ipc://", config.GetProperty<string>("session")));
    unsigned long constexpr zero(0), one(1);
    auto factory(TransportFactory::CreateTransportFactory(transport, tools::Uuid(), &config));

    Channel ch1("ch1", "pair", factory);
    ASSERT_EQ(ch1.GetNumberOfConnectedPeers(), zero);
    ch1.Bind(address);
    ASSERT_EQ(ch1.GetNumberOfConnectedPeers(), zero);

    {
        Channel ch2("ch2", "pair", factory);
        ASSERT_EQ(ch2.GetNumberOfConnectedPeers(), zero);
        ch2.Connect(address);
        std::this_thread::sleep_for(10ms);
        ASSERT_EQ(ch1.GetNumberOfConnectedPeers(), one);
        ASSERT_EQ(ch2.GetNumberOfConnectedPeers(), one);
    }

    std::this_thread::sleep_for(10ms);
    ASSERT_EQ(ch1.GetNumberOfConnectedPeers(), zero);
}

TEST(Channel, GetNumberOfConnectedPeers_zeromq)
{
    testConnectedPeers("zeromq");
}

TEST(Channel, GetNumberOfConnectedPeers_shmem)
{
    testConnectedPeers("shmem");
}

} /* namespace */
