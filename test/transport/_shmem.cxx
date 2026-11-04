/********************************************************************************
 *    Copyright (C) 2021 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <fairmq/ProgOptions.h>
#include <fairmq/shmem/Monitor.h>
#include <fairmq/tools/Unique.h>
#include <fairmq/TransportFactory.h>

#include <helper/Transport.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

#include <unistd.h> // getpid

namespace
{

using namespace std;
using namespace fair::mq;

void GetFreeMemory()
{
    ProgOptions config;
    string const sessionId(test::Configure(config));

    ASSERT_THROW(shmem::Monitor::GetFreeMemory(shmem::SessionId{sessionId}, 0), shmem::Monitor::MonitorError);

    auto factory = TransportFactory::CreateTransportFactory("shmem", tools::Uuid(), &config);

    ASSERT_NO_THROW(shmem::Monitor::GetFreeMemory(shmem::SessionId{sessionId}, 0));
    ASSERT_THROW(shmem::Monitor::GetFreeMemory(shmem::SessionId{sessionId}, 1), shmem::Monitor::MonitorError);
}

TEST(Monitor, GetFreeMemory)
{
    GetFreeMemory();
}

auto ShmConfig(ProgOptions& config, bool noCleanup) -> shmem::SessionId
{
    shmem::SessionId const session{test::Configure(config)};
    // Without the monitor: it would clean up the segment of a closed factory.
    config.SetProperty<bool>("shm-monitor", false);
    config.SetProperty<bool>("shm-no-cleanup", noCleanup);
    return session;
}

TEST(Monitor, SegmentIsPresent)
{
    ProgOptions config;
    auto const session = ShmConfig(config, false);

    EXPECT_FALSE(shmem::Monitor::SegmentIsPresent(session, 0));
    {
        auto factory = TransportFactory::CreateTransportFactory("shmem", tools::Uuid(), &config);
        EXPECT_TRUE(shmem::Monitor::SegmentIsPresent(session, 0));
        EXPECT_FALSE(shmem::Monitor::SegmentIsPresent(session, 1));
    }
    EXPECT_FALSE(shmem::Monitor::SegmentIsPresent(session, 0));
}

TEST(Monitor, RegionIsPresent)
{
    ProgOptions config;
    auto const session = ShmConfig(config, false);
    auto factory = TransportFactory::CreateTransportFactory("shmem", tools::Uuid(), &config);

    uint16_t id = 0;
    {
        auto region = factory->CreateUnmanagedRegion(10000, [](void*, size_t, void*) {});
        id = region->GetId();
        EXPECT_TRUE(shmem::Monitor::RegionIsPresent(session, id));
    }
    EXPECT_FALSE(shmem::Monitor::RegionIsPresent(session, id));
}

TEST(Monitor, CleanupAfterNoCleanup)
{
    ProgOptions config;
    auto const session = ShmConfig(config, true);
    {
        auto factory = TransportFactory::CreateTransportFactory("shmem", tools::Uuid(), &config);
        auto msg = factory->CreateMessage(1000);
    }
    // shm-no-cleanup: the segment stays after the factory is gone.
    ASSERT_TRUE(shmem::Monitor::SegmentIsPresent(session, 0));

    auto const removed = shmem::Monitor::Cleanup(session, false);
    EXPECT_FALSE(removed.empty());
    EXPECT_FALSE(shmem::Monitor::SegmentIsPresent(session, 0));
}

#ifdef FAIRMQ_DEBUG_MODE
TEST(Monitor, GetDebugInfo)
{
    ProgOptions config;
    auto const session = ShmConfig(config, false);
    auto factory = TransportFactory::CreateTransportFactory("shmem", tools::Uuid(), &config);
    auto msg1 = factory->CreateMessage(100);
    auto msg2 = factory->CreateMessage(200);

    auto const info = shmem::Monitor::GetDebugInfo(session);
    ASSERT_EQ(info.count(0), 1U);
    vector<size_t> sizes;
    for (auto const& buffer : info.at(0)) {
        EXPECT_EQ(buffer.fPid, getpid());
        sizes.push_back(buffer.fSize);
    }
    sort(sizes.begin(), sizes.end());
    EXPECT_EQ(sizes, (vector<size_t>{100, 200}));
}
#endif

} // namespace
