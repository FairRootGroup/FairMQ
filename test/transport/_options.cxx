/********************************************************************************
 *    Copyright (C) 2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <gtest/gtest.h>
#include <FairMQChannel.h>
#include <FairMQParts.h>
#include <FairMQLogger.h>
#include <FairMQTransportFactory.h>
#include <fairmq/tools/Unique.h>
#include <fairmq/ProgOptions.h>

#include <algorithm>
#include <memory>
#include <sstream>
#include <string>
#include <thread>

namespace
{

using namespace std;

void CheckOldOptionInterface(FairMQChannel& channel, const string& option)
{
    int value = 500;
    channel.GetSocket().SetOption(option, &value, sizeof(value));
    value = 0;
    size_t valueSize = sizeof(value);
    channel.GetSocket().GetOption(option, &value, &valueSize);
    ASSERT_EQ(value, 500);
}

void RunOptionsTest(const string& transport)
{
    size_t session{fair::mq::tools::UuidHash()};

    fair::mq::ProgOptions config;
    config.SetProperty<string>("session", to_string(session));
    auto factory = FairMQTransportFactory::CreateTransportFactory(transport, fair::mq::tools::Uuid(), &config);
    FairMQChannel channel("Push", "push", factory);

    CheckOldOptionInterface(channel, "linger");
    CheckOldOptionInterface(channel, "snd-hwm");
    CheckOldOptionInterface(channel, "rcv-hwm");
    CheckOldOptionInterface(channel, "snd-size");
    CheckOldOptionInterface(channel, "rcv-size");

    channel.GetSocket().SetLinger(300);
    ASSERT_EQ(channel.GetSocket().GetLinger(), 300);

    channel.GetSocket().SetSndBufSize(500);
    ASSERT_EQ(channel.GetSocket().GetSndBufSize(), 500);

    channel.GetSocket().SetRcvBufSize(500);
    ASSERT_EQ(channel.GetSocket().GetRcvBufSize(), 500);

    channel.GetSocket().SetSndKernelSize(8000);
    ASSERT_EQ(channel.GetSocket().GetSndKernelSize(), 8000);

    channel.GetSocket().SetRcvKernelSize(8000);
    ASSERT_EQ(channel.GetSocket().GetRcvKernelSize(), 8000);
}

void ZeroingAndMlock(const string& transport)
{
    size_t session{fair::mq::tools::UuidHash()};

    fair::mq::ProgOptions config;
    config.SetProperty<string>("session", to_string(session));
    config.SetProperty<size_t>("shm-segment-size", 16384);
    config.SetProperty<bool>("shm-zero-segment", true);
    config.SetProperty<bool>("shm-mlock-segment", true);

    auto factory = FairMQTransportFactory::CreateTransportFactory(transport, fair::mq::tools::Uuid(), &config);

    FairMQMessagePtr outMsg(factory->CreateMessage(10000));
    char test[10000];
    memset(test, 0, sizeof(test));
    ASSERT_EQ(memcmp(test, outMsg->GetData(), outMsg->GetSize()), 0);
}

void ZeroingAndMlockOnCreation(const string& transport)
{
    size_t session{fair::mq::tools::UuidHash()};

    fair::mq::ProgOptions config;
    config.SetProperty<string>("session", to_string(session));
    config.SetProperty<size_t>("shm-segment-size", 16384);
    config.SetProperty<bool>("shm-mlock-segment-on-creation", true);
    config.SetProperty<bool>("shm-zero-segment-on-creation", true);

    auto factory = FairMQTransportFactory::CreateTransportFactory(transport, fair::mq::tools::Uuid(), &config);

    FairMQMessagePtr outMsg(factory->CreateMessage(10000));
    char test[10000];
    memset(test, 0, sizeof(test));
    ASSERT_EQ(memcmp(test, outMsg->GetData(), outMsg->GetSize()), 0);
}

TEST(Options, zeromq)
{
    RunOptionsTest("zeromq");
}

TEST(Options, shmem)
{
    RunOptionsTest("shmem");
}

TEST(ZeroingAndMlock, shmem)
{
    ZeroingAndMlock("shmem");
}

TEST(ZeroingAndMlockOnCreation, shmem)
{
    ZeroingAndMlockOnCreation("shmem");
}

} // namespace
