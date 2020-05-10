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
#include <fairmq/Tools.h>

#include <gtest/gtest.h>

#include <string>

namespace
{

using namespace std;

void RegionEventSubscriptions(const string& transport)
{
    size_t session{fair::mq::tools::UuidHash()};

    fair::mq::ProgOptions config;
    config.SetProperty<string>("session", to_string(session));

    auto factory = FairMQTransportFactory::CreateTransportFactory(transport, fair::mq::tools::Uuid(), &config);

    constexpr int size1 = 1000000;
    constexpr int size2 = 5000000;
    constexpr int64_t userFlags = 12345;
    fair::mq::tools::SharedSemaphore blocker;

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
        factory->SubscribeToRegionEvents([&](FairMQRegionInfo info) {
            LOG(warn) << ">>>" << info.event;
            LOG(warn) << "id: " << info.id;
            LOG(warn) << "ptr: " << info.ptr;
            LOG(warn) << "size: " << info.size;
            LOG(warn) << "flags: " << info.flags;
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

TEST(EventSubscriptions, zeromq)
{
    RegionEventSubscriptions("zeromq");
}

TEST(EventSubscriptions, shmem)
{
    RegionEventSubscriptions("shmem");
}

} // namespace
