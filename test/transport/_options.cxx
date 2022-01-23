/********************************************************************************
 * Copyright (C) 2017-2021 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <algorithm>
#include <array>
#include <fairlogger/Logger.h>
#include <fairmq/Channel.h>
#include <fairmq/Parts.h>
#include <fairmq/ProgOptions.h>
#include <fairmq/TransportFactory.h>
#include <fairmq/tools/Unique.h>
#include <gtest/gtest.h>
#include <memory>
#include <sstream>
#include <string>
#include <thread>

namespace
{

using namespace std;
using namespace fair::mq;

void CheckOldOptionInterface(Channel& channel, const string& option)
{
    int const expectedValue{500};
    int value = expectedValue;
    channel.GetSocket().SetOption(option, &value, sizeof(value));
    value = 0;
    size_t valueSize = sizeof(value);
    channel.GetSocket().GetOption(option, &value, &valueSize);
    ASSERT_EQ(value, expectedValue);
}

void RunOptionsTest(const string& transport)
{
    ProgOptions config;
    config.SetProperty<string>("session", tools::Uuid());
    config.SetProperty<size_t>("shm-segment-size", 100000000);
    auto factory = TransportFactory::CreateTransportFactory(transport, tools::Uuid(), &config);
    Channel channel("Push", "push", factory);

    CheckOldOptionInterface(channel, "linger");
    CheckOldOptionInterface(channel, "snd-hwm");
    CheckOldOptionInterface(channel, "rcv-hwm");
    CheckOldOptionInterface(channel, "snd-size");
    CheckOldOptionInterface(channel, "rcv-size");

    size_t const linger{300};
    channel.GetSocket().SetLinger(linger);
    ASSERT_EQ(channel.GetSocket().GetLinger(), linger);

    size_t const bufSize{500};
    channel.GetSocket().SetSndBufSize(bufSize);
    ASSERT_EQ(channel.GetSocket().GetSndBufSize(), bufSize);
    channel.GetSocket().SetRcvBufSize(bufSize);
    ASSERT_EQ(channel.GetSocket().GetRcvBufSize(), bufSize);

    size_t const kernelSize{8000};
    channel.GetSocket().SetSndKernelSize(kernelSize);
    ASSERT_EQ(channel.GetSocket().GetSndKernelSize(), kernelSize);
    channel.GetSocket().SetRcvKernelSize(kernelSize);
    ASSERT_EQ(channel.GetSocket().GetRcvKernelSize(), kernelSize);
}

void ZeroingAndMlock(const string& transport)
{
    ProgOptions config;
    config.SetProperty<string>("session", tools::Uuid());
    config.SetProperty<size_t>("shm-segment-size", 16384); // NOLINT
    config.SetProperty<bool>("shm-zero-segment", true);
    config.SetProperty<bool>("shm-mlock-segment", true);

    auto factory = TransportFactory::CreateTransportFactory(transport, tools::Uuid(), &config);

    constexpr size_t size{10000};
    auto outMsg(factory->CreateMessage(size));
    array<char, size> test{0};
    ASSERT_EQ(memcmp(test.data(), outMsg->GetData(), outMsg->GetSize()), 0);
}

void ZeroingAndMlockOnCreation(const string& transport)
{
    size_t session{tools::UuidHash()};

    ProgOptions config;
    config.SetProperty<string>("session", to_string(session));
    config.SetProperty<size_t>("shm-segment-size", 16384); // NOLINT
    config.SetProperty<bool>("shm-mlock-segment-on-creation", true);
    config.SetProperty<bool>("shm-zero-segment-on-creation", true);

    auto factory = TransportFactory::CreateTransportFactory(transport, tools::Uuid(), &config);

    constexpr size_t size{10000};
    auto outMsg(factory->CreateMessage(size));
    array<char, size> test{0};
    ASSERT_EQ(memcmp(test.data(), outMsg->GetData(), outMsg->GetSize()), 0);
}

TEST(Options, zeromq) // NOLINT
{
    RunOptionsTest("zeromq");
}

TEST(Options, shmem) // NOLINT
{
    RunOptionsTest("shmem");
}

TEST(ZeroingAndMlock, shmem) // NOLINT
{
    ZeroingAndMlock("shmem");
}

TEST(ZeroingAndMlockOnCreation, shmem)
{
    ZeroingAndMlockOnCreation("shmem");
}

} // namespace
