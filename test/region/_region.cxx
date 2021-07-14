/********************************************************************************
 *    Copyright (C) 2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <FairMQLogger.h>
#include <FairMQTransportFactory.h>
#include <fairmq/ProgOptions.h>
#include <fairmq/tools/Unique.h>
#include <fairmq/tools/Semaphore.h>
#include <fairmq/tools/Strings.h>

#include <gtest/gtest.h>

#include <memory> // make_unique
#include <string>

namespace
{

using namespace std;

void RegionsCache(const string& transport, const string& address)
{
    size_t session1 = fair::mq::tools::UuidHash();
    size_t session2 = fair::mq::tools::UuidHash();

    fair::mq::ProgOptions config1;
    fair::mq::ProgOptions config2;
    config1.SetProperty<string>("session", to_string(session1));
    config2.SetProperty<string>("session", to_string(session2));

    auto factory1 = FairMQTransportFactory::CreateTransportFactory(transport, fair::mq::tools::Uuid(), &config1);
    auto factory2 = FairMQTransportFactory::CreateTransportFactory(transport, fair::mq::tools::Uuid(), &config2);

    auto region1 = factory1->CreateUnmanagedRegion(1000000, [](void*, size_t, void*) {});
    auto region2 = factory2->CreateUnmanagedRegion(1000000, [](void*, size_t, void*) {});
    void* r1ptr = region1->GetData();
    void* r2ptr = region2->GetData();

    FairMQChannel push1("Push1", "push", factory1);
    FairMQChannel pull1("Pull1", "pull", factory1);
    push1.Bind(address + to_string(1));
    pull1.Connect(address + to_string(1));
    FairMQChannel push2("Push2", "push", factory2);
    FairMQChannel pull2("Pull2", "pull", factory2);
    push2.Bind(address + to_string(2));
    pull2.Connect(address + to_string(2));

    {
        static_cast<char*>(r1ptr)[0] = 97; // a
        static_cast<char*>(static_cast<char*>(r1ptr) + 100)[0] = 98; // b
        static_cast<char*>(r2ptr)[0] = 99; // c
        static_cast<char*>(static_cast<char*>(r2ptr) + 100)[0] = 100; // d

        FairMQMessagePtr m1(push1.NewMessage(region1, r1ptr, 100, nullptr));
        FairMQMessagePtr m2(push1.NewMessage(region1, static_cast<char*>(r1ptr) + 100, 100, nullptr));
        push1.Send(m1);
        push1.Send(m2);

        FairMQMessagePtr m3(push2.NewMessage(region2, r2ptr, 100, nullptr));
        FairMQMessagePtr m4(push2.NewMessage(region2, static_cast<char*>(r2ptr) + 100, 100, nullptr));
        push2.Send(m3);
        push2.Send(m4);
    }

    {
        FairMQMessagePtr m1(pull1.NewMessage());
        FairMQMessagePtr m2(pull1.NewMessage());
        ASSERT_EQ(pull1.Receive(m1), 100);
        ASSERT_EQ(pull1.Receive(m2), 100);
        ASSERT_EQ(static_cast<char*>(m1->GetData())[0], 'a');
        ASSERT_EQ(static_cast<char*>(m2->GetData())[0], 'b');

        FairMQMessagePtr m3(pull2.NewMessage());
        FairMQMessagePtr m4(pull2.NewMessage());
        ASSERT_EQ(pull2.Receive(m3), 100);
        ASSERT_EQ(pull2.Receive(m4), 100);
        ASSERT_EQ(static_cast<char*>(m3->GetData())[0], 'c');
        ASSERT_EQ(static_cast<char*>(m4->GetData())[0], 'd');
    }
}

void RegionEventSubscriptions(const string& transport)
{
    size_t session{fair::mq::tools::UuidHash()};

    fair::mq::ProgOptions config;
    config.SetProperty<string>("session", to_string(session));

    auto factory = FairMQTransportFactory::CreateTransportFactory(transport, fair::mq::tools::Uuid(), &config);

    constexpr int size1 = 1000000;
    constexpr int size2 = 5000000;
    constexpr int64_t userFlags = 12345;
    fair::mq::tools::Semaphore blocker;

    {
        auto region1 = factory->CreateUnmanagedRegion(size1, [](void*, size_t, void*) {});
        void* ptr1 = region1->GetData();
        uint64_t id1 = region1->GetId();
        ASSERT_EQ(region1->GetSize(), size1);

        auto region2 = factory->CreateUnmanagedRegion(size2, userFlags, [](void*, size_t, void*) {});
        void* ptr2 = region2->GetData();
        uint64_t id2 = region2->GetId();
        ASSERT_EQ(region2->GetSize(), size2);

        ASSERT_EQ(factory->SubscribedToRegionEvents(), false);
        factory->SubscribeToRegionEvents([&, id1, id2, ptr1, ptr2](FairMQRegionInfo info) {
            LOG(info) << ">>> " << info.event << ": "
                      << (info.managed ? "managed" : "unmanaged")
                      << ", id: " << info.id
                      << ", ptr: " << info.ptr
                      << ", size: " << info.size
                      << ", flags: " << info.flags;
            if (info.event == FairMQRegionEvent::created) {
                if (info.id == id1) {
                    ASSERT_EQ(info.size, size1);
                    ASSERT_EQ(info.ptr, ptr1);
                    blocker.Signal();
                } else if (info.id == id2) {
                    ASSERT_EQ(info.size, size2);
                    ASSERT_EQ(info.ptr, ptr2);
                    ASSERT_EQ(info.flags, userFlags);
                    blocker.Signal();
                }
            } else if (info.event == FairMQRegionEvent::destroyed) {
                if (info.id == id1) {
                    blocker.Signal();
                } else if (info.id == id2) {
                    blocker.Signal();
                }
            }
        });
        ASSERT_EQ(factory->SubscribedToRegionEvents(), true);

        LOG(info) << "waiting for blockers...";
        blocker.Wait();
        LOG(info) << "1 done.";
        blocker.Wait();
        LOG(info) << "2 done.";
    }

    blocker.Wait();
    LOG(info) << "3 done.";
    blocker.Wait();
    LOG(info) << "4 done.";
    LOG(info) << "All done.";

    factory->UnsubscribeFromRegionEvents();
    ASSERT_EQ(factory->SubscribedToRegionEvents(), false);
}

void RegionCallbacks(const string& transport, const string& _address)
{
    size_t session(fair::mq::tools::UuidHash());
    std::string address(fair::mq::tools::ToString(_address, "_", transport));

    fair::mq::ProgOptions config;
    config.SetProperty<string>("session", to_string(session));

    auto factory = FairMQTransportFactory::CreateTransportFactory(transport, fair::mq::tools::Uuid(), &config);

    unique_ptr<int> intPtr1 = make_unique<int>(42);
    unique_ptr<int> intPtr2 = make_unique<int>(43);
    fair::mq::tools::Semaphore blocker;

    FairMQChannel push("Push", "push", factory);
    push.Bind(address);

    FairMQChannel pull("Pull", "pull", factory);
    pull.Connect(address);

    void* ptr1 = nullptr;
    size_t size1 = 100;
    void* ptr2 = nullptr;
    size_t size2 = 200;

    auto region1 = factory->CreateUnmanagedRegion(2000000, [&](void* ptr, size_t size, void* hint) {
        ASSERT_EQ(ptr, ptr1);
        ASSERT_EQ(size, size1);
        ASSERT_EQ(hint, intPtr1.get());
        ASSERT_EQ(*static_cast<int*>(hint), 42);
        blocker.Signal();
    });
    ptr1 = region1->GetData();

    auto region2 = factory->CreateUnmanagedRegion(3000000, [&](const std::vector<fair::mq::RegionBlock>& blocks) {
        ASSERT_EQ(blocks.size(), 1);
        ASSERT_EQ(blocks.at(0).ptr, ptr2);
        ASSERT_EQ(blocks.at(0).size, size2);
        ASSERT_EQ(blocks.at(0).hint, intPtr2.get());
        ASSERT_EQ(*static_cast<int*>(blocks.at(0).hint), 43);
        blocker.Signal();
    });
    ptr2 = region2->GetData();

    {
        FairMQMessagePtr msg1out(push.NewMessage(region1, ptr1, size1, intPtr1.get()));
        FairMQMessagePtr msg2out(push.NewMessage(region2, ptr2, size2, intPtr2.get()));
        ASSERT_EQ(push.Send(msg1out), size1);
        ASSERT_EQ(push.Send(msg2out), size2);
    }

    {
        FairMQMessagePtr msg1in(pull.NewMessage());
        FairMQMessagePtr msg2in(pull.NewMessage());
        ASSERT_EQ(pull.Receive(msg1in), size1);
        ASSERT_EQ(pull.Receive(msg2in), size2);
    }

    LOG(info) << "waiting for blockers...";
    blocker.Wait();
    LOG(info) << "1 done.";
    blocker.Wait();
    LOG(info) << "2 done.";
}

TEST(Cache, zeromq)
{
    RegionsCache("zeromq", "ipc://test_region_cache");
}

TEST(Cache, shmem)
{
    RegionsCache("shmem", "ipc://test_region_cache");
}

TEST(EventSubscriptions, zeromq)
{
    RegionEventSubscriptions("zeromq");
}

TEST(EventSubscriptions, shmem)
{
    RegionEventSubscriptions("shmem");
}

TEST(Callbacks, zeromq)
{
    RegionCallbacks("zeromq", "ipc://test_region_callbacks");
}

TEST(Callbacks, shmem)
{
    RegionCallbacks("shmem", "ipc://test_region_callbacks");
}

} // namespace
