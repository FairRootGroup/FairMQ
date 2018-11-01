/********************************************************************************
 *    Copyright (C) 2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <FairMQChannel.h>

#include <gtest/gtest.h>

#include <string>

namespace
{

using namespace std;
using namespace fair::mq;

TEST(Channel, Validation)
{
    FairMQChannel channel;
    ASSERT_THROW(channel.ValidateChannel(), FairMQChannel::ChannelConfigurationError);

    channel.UpdateType("pair");
    ASSERT_EQ(channel.ValidateChannel(), false);
    ASSERT_EQ(channel.IsValid(), false);

    channel.UpdateAddress("bla");
    ASSERT_THROW(channel.ValidateChannel(), FairMQChannel::ChannelConfigurationError);

    channel.UpdateMethod("connect");
    ASSERT_EQ(channel.ValidateChannel(), false);
    ASSERT_EQ(channel.IsValid(), false);

    channel.UpdateAddress("ipc://");
    ASSERT_EQ(channel.ValidateChannel(), false);
    ASSERT_EQ(channel.IsValid(), false);

    channel.UpdateAddress("verbs://");
    ASSERT_EQ(channel.ValidateChannel(), false);
    ASSERT_EQ(channel.IsValid(), false);

    channel.UpdateAddress("inproc://");
    ASSERT_EQ(channel.ValidateChannel(), false);
    ASSERT_EQ(channel.IsValid(), false);

    channel.UpdateAddress("tcp://");
    ASSERT_EQ(channel.ValidateChannel(), false);
    ASSERT_EQ(channel.IsValid(), false);

    channel.UpdateAddress("tcp://localhost:5555");
    ASSERT_EQ(channel.ValidateChannel(), true);
    ASSERT_EQ(channel.IsValid(), true);

    channel.UpdateSndBufSize(-1);
    ASSERT_THROW(channel.ValidateChannel(), FairMQChannel::ChannelConfigurationError);
    channel.UpdateSndBufSize(1000);
    ASSERT_NO_THROW(channel.ValidateChannel());

    channel.UpdateRcvBufSize(-1);
    ASSERT_THROW(channel.ValidateChannel(), FairMQChannel::ChannelConfigurationError);
    channel.UpdateRcvBufSize(1000);
    ASSERT_NO_THROW(channel.ValidateChannel());

    channel.UpdateSndKernelSize(-1);
    ASSERT_THROW(channel.ValidateChannel(), FairMQChannel::ChannelConfigurationError);
    channel.UpdateSndKernelSize(1000);
    ASSERT_NO_THROW(channel.ValidateChannel());

    channel.UpdateRcvKernelSize(-1);
    ASSERT_THROW(channel.ValidateChannel(), FairMQChannel::ChannelConfigurationError);
    channel.UpdateRcvKernelSize(1000);
    ASSERT_NO_THROW(channel.ValidateChannel());

    channel.UpdateRateLogging(-1);
    ASSERT_THROW(channel.ValidateChannel(), FairMQChannel::ChannelConfigurationError);
    channel.UpdateRateLogging(1);
    ASSERT_NO_THROW(channel.ValidateChannel());

    FairMQChannel channel2 = channel;
    ASSERT_NO_THROW(channel2.ValidateChannel());
    ASSERT_EQ(channel2.ValidateChannel(), true);
    ASSERT_EQ(channel2.IsValid(), true);
    ASSERT_EQ(channel2.ValidateChannel(), true);

    channel2.UpdateChannelName("Kanal");
    ASSERT_EQ(channel2.GetChannelName(), "Kanal");

    channel2.ResetChannel();
    ASSERT_EQ(channel2.IsValid(), false);
    ASSERT_EQ(channel2.ValidateChannel(), true);
}

} /* namespace */
