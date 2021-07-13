/********************************************************************************
 * Copyright (C) 2018-2021 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <fairmq/Channel.h>
#include <gtest/gtest.h>
#include <string>

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

} /* namespace */
