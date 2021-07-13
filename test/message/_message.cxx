/********************************************************************************
 * Copyright (C) 2017-2021 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <array>
#include <cassert>
#include <cstdint>
#include <fairlogger/Logger.h>
#include <fairmq/Channel.h>
#include <fairmq/ProgOptions.h>
#include <fairmq/TransportFactory.h>
#include <fairmq/tools/Strings.h>
#include <fairmq/tools/Unique.h>
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

namespace
{

using namespace std;
using namespace fair::mq;

auto AsStringView(Message const& msg) -> string_view
{
    return {static_cast<char const*>(msg.GetData()), msg.GetSize()};
}

auto RunPushPullWithMsgResize(string const & transport, string const & _address) -> void
{
    ProgOptions config;
    config.SetProperty<string>("session", tools::Uuid());
    auto factory(TransportFactory::CreateTransportFactory(transport, tools::Uuid(), &config));

    Channel push{"Push", "push", factory};
    Channel pull{"Pull", "pull", factory};
    auto const address(tools::ToString(_address, "_", transport));
    push.Bind(address);
    pull.Connect(address);

    {
        size_t const size{6};
        auto outMsg(push.NewMessage(size));
        ASSERT_EQ(outMsg->GetSize(), size);
        memcpy(outMsg->GetData(), "ABCDEF", size);
        ASSERT_TRUE(outMsg->SetUsedSize(5));
        ASSERT_TRUE(outMsg->SetUsedSize(5));
        ASSERT_FALSE(outMsg->SetUsedSize(7));
        ASSERT_EQ(outMsg->GetSize(), 5);

        // check if the data is still intact
        ASSERT_EQ(AsStringView(*outMsg)[0], 'A');
        ASSERT_EQ(AsStringView(*outMsg)[1], 'B');
        ASSERT_EQ(AsStringView(*outMsg)[2], 'C');
        ASSERT_EQ(AsStringView(*outMsg)[3], 'D');
        ASSERT_EQ(AsStringView(*outMsg)[4], 'E');
        ASSERT_TRUE(outMsg->SetUsedSize(2));
        ASSERT_EQ(outMsg->GetSize(), 2);
        ASSERT_EQ(AsStringView(*outMsg)[0], 'A');
        ASSERT_EQ(AsStringView(*outMsg)[1], 'B');

        auto msgCopy(push.NewMessage());
        msgCopy->Copy(*outMsg);
        ASSERT_EQ(msgCopy->GetSize(), 2);

        ASSERT_EQ(push.Send(outMsg), 2);

        auto inMsg(pull.NewMessage());
        ASSERT_EQ(pull.Receive(inMsg), 2);
        ASSERT_EQ(inMsg->GetSize(), 2);
        ASSERT_EQ(AsStringView(*inMsg)[0], 'A');
        ASSERT_EQ(AsStringView(*inMsg)[1], 'B');
    }

    {
        size_t const size{1000};
        auto outMsg(push.NewMessage(size));
        ASSERT_TRUE(outMsg->SetUsedSize(0));
        ASSERT_EQ(outMsg->GetSize(), 0);
        auto msgCopy(push.NewMessage());
        msgCopy->Copy(*outMsg);
        ASSERT_EQ(msgCopy->GetSize(), 0);
        ASSERT_EQ(push.Send(outMsg), 0);
        auto inMsg(pull.NewMessage());
        ASSERT_EQ(pull.Receive(inMsg), 0);
        ASSERT_EQ(inMsg->GetSize(), 0);
    }
}

auto RunMsgRebuild(const string& transport) -> void
{
    ProgOptions config;
    config.SetProperty<string>("session", tools::Uuid());
    auto factory(TransportFactory::CreateTransportFactory(transport, tools::Uuid(), &config));

    size_t const msgSize{100};
    string const expectedStr{"asdf"};
    auto msg(factory->CreateMessage());
    EXPECT_EQ(msg->GetSize(), 0);
    msg->Rebuild(msgSize);
    EXPECT_EQ(msg->GetSize(), msgSize);

    auto str(make_unique<string>(expectedStr));
    void* data(str->data());
    auto const size(str->length());
    msg->Rebuild(
        data,
        size,
        [](void* /*data*/, void* obj) { delete static_cast<string*>(obj); }, // NOLINT
        str.release());
    EXPECT_NE(msg->GetSize(), msgSize);
    EXPECT_EQ(msg->GetSize(), expectedStr.length());
    EXPECT_EQ(AsStringView(*msg), expectedStr);
}

auto CheckMsgAlignment(Message const& msg, fair::mq::Alignment alignment) -> bool
{
    assert(static_cast<size_t>(alignment) > 0); // NOLINT
    return (reinterpret_cast<uintptr_t>(msg.GetData()) % static_cast<size_t>(alignment)) == 0; // NOLINT
}

auto RunPushPullWithAlignment(string const& transport, string const& _address) -> void
{
    ProgOptions config;
    config.SetProperty<string>("session", tools::Uuid());
    auto factory(TransportFactory::CreateTransportFactory(transport, tools::Uuid(), &config));

    Channel push{"Push", "push", factory};
    Channel pull{"Pull", "pull", factory};
    auto const address(tools::ToString(_address, "_", transport));
    push.Bind(address);
    pull.Connect(address);

    {
        Alignment const align{64};
        for (size_t const size : {100, 32}) {
            auto outMsg(push.NewMessage(size, align));
            auto inMsg(pull.NewMessage(align));

            ASSERT_TRUE(CheckMsgAlignment(*outMsg, align));
            ASSERT_EQ(push.Send(outMsg), size);
            ASSERT_EQ(pull.Receive(inMsg), size);
            ASSERT_TRUE(CheckMsgAlignment(*inMsg, align));
        }
    }

    {
        Alignment const align{0};
        size_t const size{100};
        auto outMsg(push.NewMessage(size, align));
        auto inMsg(pull.NewMessage(align));

        ASSERT_EQ(push.Send(outMsg), size);
        ASSERT_EQ(pull.Receive(inMsg), size);
    }

    for (auto const align : {Alignment{64}, Alignment{32}}) {
        size_t const size25{25};
        size_t const size50{50};

        auto msg(push.NewMessage(size25));
        msg->Rebuild(size50, align);
        ASSERT_TRUE(CheckMsgAlignment(*msg, align));
    }
}

auto EmptyMessage(string const& transport, string const& _address) -> void
{
    ProgOptions config;
    config.SetProperty<string>("session", tools::Uuid());
    auto factory(TransportFactory::CreateTransportFactory(transport, tools::Uuid(), &config));

    Channel push{"Push", "push", factory};
    Channel pull{"Pull", "pull", factory};
    auto const address(tools::ToString(_address, "_", transport));
    push.Bind(address);
    pull.Connect(address);

    auto outMsg(push.NewMessage());
    ASSERT_EQ(outMsg->GetData(), nullptr);
    ASSERT_EQ(push.Send(outMsg), 0);

    auto inMsg(pull.NewMessage());
    ASSERT_EQ(pull.Receive(inMsg), 0);
    ASSERT_EQ(inMsg->GetData(), nullptr);
}

TEST(Resize, zeromq) // NOLINT
{
    RunPushPullWithMsgResize("zeromq", "ipc://test_message_resize");
}

TEST(Resize, shmem) // NOLINT
{
    RunPushPullWithMsgResize("shmem", "ipc://test_message_resize");
}

TEST(Rebuild, zeromq) // NOLINT
{
    RunMsgRebuild("zeromq");
}

TEST(Rebuild, shmem) // NOLINT
{
    RunMsgRebuild("shmem");
}

TEST(Alignment, shmem) // NOLINT
{
    RunPushPullWithAlignment("shmem", "ipc://test_message_alignment");
}

TEST(Alignment, zeromq) // NOLINT
{
    RunPushPullWithAlignment("zeromq", "ipc://test_message_alignment");
}

TEST(EmptyMessage, zeromq) // NOLINT
{
    EmptyMessage("zeromq", "ipc://test_empty_message");
}

TEST(EmptyMessage, shmem) // NOLINT
{
    EmptyMessage("shmem", "ipc://test_empty_message");
}

} // namespace
