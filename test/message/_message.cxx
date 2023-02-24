/********************************************************************************
 * Copyright (C) 2017-2021 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <fairmq/Channel.h>
#include <fairmq/ProgOptions.h>
#include <fairmq/tools/Semaphore.h>
#include <fairmq/tools/Strings.h>
#include <fairmq/tools/Unique.h>
#include <fairmq/TransportFactory.h>
#include <fairmq/shmem/Message.h>

#include <fairlogger/Logger.h>

#include <gtest/gtest.h>

#include <array>
#include <cassert>
#include <cstdint>
#include <memory>
#include <string_view>
#include <string>
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
    config.SetProperty<size_t>("shm-segment-size", 100000000);
    config.SetProperty<bool>("shm-monitor", true);
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
    config.SetProperty<size_t>("shm-segment-size", 100000000);
    config.SetProperty<bool>("shm-monitor", true);
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
    config.SetProperty<size_t>("shm-segment-size", 100000000);
    config.SetProperty<bool>("shm-monitor", true);
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

    size_t const size25{25};
    size_t const size50{50};
    Alignment const align64{64};

    auto msg1(push.NewMessage(size25));
    msg1->Rebuild(size50, align64);
    ASSERT_TRUE(CheckMsgAlignment(*msg1, align64));

    Alignment const align32{32};
    auto msg2(push.NewMessage(size25, align64));
    msg2->Rebuild(size50, align32);
    ASSERT_TRUE(CheckMsgAlignment(*msg2, align32));

    auto msgCopy(push.NewMessage());
    msgCopy->Copy(*msg2);
    ASSERT_TRUE(CheckMsgAlignment(*msgCopy, align32));
}

auto EmptyMessage(string const& transport, string const& _address) -> void
{
    ProgOptions config;
    config.SetProperty<string>("session", tools::Uuid());
    config.SetProperty<size_t>("shm-segment-size", 100000000);
    config.SetProperty<bool>("shm-monitor", true);
    auto factory(TransportFactory::CreateTransportFactory(transport, tools::Uuid(), &config));

    Channel push{"Push", "push", factory};
    Channel pull{"Pull", "pull", factory};
    auto const address(tools::ToString(_address, "_", transport));
    push.Bind(address);
    pull.Connect(address);

    {
        auto outMsg(push.NewMessage());
        ASSERT_EQ(outMsg->GetData(), nullptr);
        ASSERT_EQ(push.Send(outMsg), 0);

        auto inMsg(pull.NewMessage());
        ASSERT_EQ(pull.Receive(inMsg), 0);
        ASSERT_EQ(inMsg->GetData(), nullptr);
    }

    {
        auto outMsg(push.NewMessage(0));
        ASSERT_EQ(outMsg->GetSize(), 0);

        size_t const size100{100};
        outMsg->Rebuild(size100);
        ASSERT_EQ(outMsg->GetSize(), size100);

        outMsg->Rebuild(0);
        ASSERT_EQ(outMsg->GetSize(), 0);

        auto msgCopy(push.NewMessage());
        msgCopy->Copy(*outMsg);
        ASSERT_EQ(msgCopy->GetSize(), 0);
        ASSERT_EQ(push.Send(outMsg), 0);
        ASSERT_EQ(push.Send(msgCopy), 0);

        auto inMsg(pull.NewMessage());
        auto inMsg2(pull.NewMessage());
        ASSERT_EQ(pull.Receive(inMsg), 0);
        ASSERT_EQ(pull.Receive(inMsg2), 0);
        ASSERT_EQ(inMsg->GetSize(), 0);
        ASSERT_EQ(inMsg2->GetSize(), 0);
    }
}

// The "zero copy" property of the Copy() method is an implementation detail and is not guaranteed.
// Currently it holds true for the shmem (across devices) and for zeromq (within same device) transports.
auto ZeroCopy() -> void
{
    ProgOptions config;
    config.SetProperty<string>("session", tools::Uuid());
    config.SetProperty<size_t>("shm-segment-size", 100000000);
    config.SetProperty<bool>("shm-monitor", true);
    auto factory(TransportFactory::CreateTransportFactory("shmem", tools::Uuid(), &config));

    unique_ptr<string> str(make_unique<string>("asdf"));
    const size_t size = 2;
    MessagePtr original(factory->CreateMessage(size));
    memcpy(original->GetData(), "AB", size);
    {
        MessagePtr copy(factory->CreateMessage());
        copy->Copy(*original);
        EXPECT_EQ(original->GetSize(), copy->GetSize());
        EXPECT_EQ(original->GetData(), copy->GetData());
        EXPECT_EQ(static_cast<const shmem::Message&>(*original).GetRefCount(), 2);
        EXPECT_EQ(static_cast<const shmem::Message&>(*copy).GetRefCount(), 2);

        // buffer must be still intact
        ASSERT_EQ(AsStringView(*original)[0], 'A');
        ASSERT_EQ(AsStringView(*original)[1], 'B');
        ASSERT_EQ(AsStringView(*copy)[0], 'A');
        ASSERT_EQ(AsStringView(*copy)[1], 'B');
    }
    EXPECT_EQ(static_cast<const shmem::Message&>(*original).GetRefCount(), 1);
}

// The "zero copy" property of the Copy() method is an implementation detail and is not guaranteed.
// Currently it holds true for the shmem (across devices) and for zeromq (within same device) transports.
auto ZeroCopyFromUnmanaged(string const& address) -> void
{
    ProgOptions config1;
    ProgOptions config2;
    string session(tools::Uuid());
    config1.SetProperty<string>("session", session);
    config1.SetProperty<size_t>("shm-segment-size", 100000000);
    config1.SetProperty<bool>("shm-monitor", true);
    config2.SetProperty<string>("session", session);
    config2.SetProperty<size_t>("shm-segment-size", 100000000);
    config2.SetProperty<bool>("shm-monitor", true);
    // ref counts should be accessible accross different segments
    config2.SetProperty<uint16_t>("shm-segment-id", 2);
    auto factory1(TransportFactory::CreateTransportFactory("shmem", tools::Uuid(), &config1));
    auto factory2(TransportFactory::CreateTransportFactory("shmem", tools::Uuid(), &config2));

    const size_t msgSize{100};
    const size_t regionSize{1000000};
    tools::Semaphore blocker;

    auto region = factory1->CreateUnmanagedRegion(regionSize, [&blocker](void*, size_t, void*) {
        blocker.Signal();
    });

    {
        Channel push("Push", "push", factory1);
        Channel pull("Pull", "pull", factory2);

        push.Bind(address);
        pull.Connect(address);

        const size_t offset = 100;
        auto msg1(push.NewMessage(region, static_cast<char*>(region->GetData()), msgSize, nullptr));
        auto msg2(push.NewMessage(region, static_cast<char*>(region->GetData()) + offset, msgSize, nullptr));
        const size_t contentSize = 2;
        memcpy(msg1->GetData(), "AB", contentSize);
        memcpy(msg2->GetData(), "CD", contentSize);
        EXPECT_EQ(static_cast<const shmem::Message&>(*msg1).GetRefCount(), 1);

        {
            auto copyFromOriginal(push.NewMessage());
            copyFromOriginal->Copy(*msg1);
            EXPECT_EQ(static_cast<const shmem::Message&>(*msg1).GetRefCount(), 2);
            EXPECT_EQ(static_cast<const shmem::Message&>(*msg1).GetRefCount(), static_cast<const shmem::Message&>(*copyFromOriginal).GetRefCount());
            {
                auto copyFromCopy(push.NewMessage());
                copyFromCopy->Copy(*copyFromOriginal);
                EXPECT_EQ(static_cast<const shmem::Message&>(*msg1).GetRefCount(), 3);
                EXPECT_EQ(static_cast<const shmem::Message&>(*msg1).GetRefCount(), static_cast<const shmem::Message&>(*copyFromCopy).GetRefCount());

                EXPECT_EQ(msg1->GetSize(), copyFromOriginal->GetSize());
                EXPECT_EQ(msg1->GetData(), copyFromOriginal->GetData());
                EXPECT_EQ(msg1->GetSize(), copyFromCopy->GetSize());
                EXPECT_EQ(msg1->GetData(), copyFromCopy->GetData());
                EXPECT_EQ(copyFromOriginal->GetSize(), copyFromCopy->GetSize());
                EXPECT_EQ(copyFromOriginal->GetData(), copyFromCopy->GetData());

                // messing with the ref count should not have affected the user buffer
                ASSERT_EQ(AsStringView(*msg1)[0], 'A');
                ASSERT_EQ(AsStringView(*msg1)[1], 'B');

                push.Send(copyFromCopy);
                push.Send(msg2);

                auto incomingCopiedMsg(pull.NewMessage());
                auto incomingOriginalMsg(pull.NewMessage());
                pull.Receive(incomingCopiedMsg);
                pull.Receive(incomingOriginalMsg);

                EXPECT_EQ(static_cast<const shmem::Message&>(*incomingCopiedMsg).GetRefCount(), 3);
                EXPECT_EQ(static_cast<const shmem::Message&>(*incomingOriginalMsg).GetRefCount(), 1);

                ASSERT_EQ(AsStringView(*incomingCopiedMsg)[0], 'A');
                ASSERT_EQ(AsStringView(*incomingCopiedMsg)[1], 'B');

                {
                    // copying on a different segment should work
                    auto copyFromIncoming(pull.NewMessage());
                    copyFromIncoming->Copy(*incomingOriginalMsg);
                    EXPECT_EQ(static_cast<const shmem::Message&>(*copyFromIncoming).GetRefCount(), 2);

                    ASSERT_EQ(AsStringView(*incomingOriginalMsg)[0], 'C');
                    ASSERT_EQ(AsStringView(*incomingOriginalMsg)[1], 'D');
                }

                EXPECT_EQ(static_cast<const shmem::Message&>(*incomingOriginalMsg).GetRefCount(), 1);
            }
            EXPECT_EQ(static_cast<const shmem::Message&>(*msg1).GetRefCount(), 2);
        }
        EXPECT_EQ(static_cast<const shmem::Message&>(*msg1).GetRefCount(), 1);
    }

    blocker.Wait();
    blocker.Wait();
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

TEST(ZeroCopy, shmem) // NOLINT
{
    ZeroCopy();
}

TEST(ZeroCopyFromUnmanaged, shmem) // NOLINT
{
    ZeroCopyFromUnmanaged("ipc://test_zerocopy_unmanaged");
}

} // namespace
