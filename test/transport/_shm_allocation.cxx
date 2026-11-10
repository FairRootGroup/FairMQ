/********************************************************************************
 *    Copyright (C) 2026 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <fairmq/Message.h>
#include <fairmq/ProgOptions.h>
#include <fairmq/TransportFactory.h>
#include <fairmq/tools/Unique.h>

#include <helper/Sync.h>
#include <helper/Transport.h>

#include <gtest/gtest.h>

#include <chrono>
#include <cstddef>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace
{

using namespace std;
using namespace fair::mq;

constexpr size_t kSegmentSize = 1000000;
constexpr size_t kMsgSize = 100000;

/// The parameter is the shm allocation algorithm.
class ShmAllocation : public testing::TestWithParam<string>
{
  protected:
    ShmAllocation()
    {
        test::Configure(fConfig);
        fConfig.SetProperty<size_t>("shm-segment-size", kSegmentSize);
        fConfig.SetProperty<string>("shm-allocation", GetParam());
    }

    auto Factory() -> shared_ptr<TransportFactory>
    {
        return TransportFactory::CreateTransportFactory("shmem", tools::Uuid(), &fConfig);
    }

    /// Allocate messages until the segment is full (MessageBadAlloc).
    static auto Fill(TransportFactory& factory) -> vector<MessagePtr>
    {
        vector<MessagePtr> msgs;
        try {
            for (size_t i = 0; i < 2 * kSegmentSize / kMsgSize; ++i) {
                msgs.push_back(factory.CreateMessage(kMsgSize));
            }
        } catch (MessageBadAlloc const&) {
            return msgs;
        }
        ADD_FAILURE() << "the segment did not get full";
        return msgs;
    }

    ProgOptions fConfig;
};

TEST_P(ShmAllocation, MessageLargerThanTheSegmentThrows)
{
    auto factory = Factory();
    EXPECT_THROW(factory->CreateMessage(2 * kSegmentSize), MessageBadAlloc);
    // The segment is still usable.
    EXPECT_NE(factory->CreateMessage(kMsgSize), nullptr);
}

TEST_P(ShmAllocation, FullSegmentThrowsAndRecovers)
{
    auto factory = Factory();
    auto msgs = Fill(*factory);
    ASSERT_GE(msgs.size(), 5U);
    ASSERT_LT(msgs.size(), kSegmentSize / kMsgSize);

    msgs.pop_back();
    EXPECT_NE(factory->CreateMessage(kMsgSize), nullptr);
}

TEST_P(ShmAllocation, MaxAttempts)
{
    fConfig.SetProperty<int>("bad-alloc-max-attempts", 4);
    fConfig.SetProperty<int>("bad-alloc-attempt-interval", 20);
    auto factory = Factory();
    vector<MessagePtr> msgs;
    for (size_t i = 0; i < kSegmentSize / kMsgSize; ++i) {
        try {
            msgs.push_back(factory->CreateMessage(kMsgSize));
        } catch (MessageBadAlloc const&) {
            break;
        }
    }

    // A full segment: 3 sleeps of 20 ms before the 4th attempt throws.
    auto const start = chrono::steady_clock::now();
    EXPECT_THROW(factory->CreateMessage(kMsgSize), MessageBadAlloc);
    EXPECT_GE(chrono::steady_clock::now() - start, chrono::milliseconds(60));
}

TEST_P(ShmAllocation, NoThrowWaitsForFreeMemory)
{
    fConfig.SetProperty<int>("bad-alloc-attempt-interval", 5);
    auto filler = Factory();
    auto msgs = Fill(*filler);
    ASSERT_FALSE(msgs.empty());

    fConfig.SetProperty<bool>("shm-throw-bad-alloc", false);
    auto factory = Factory(); // same session and segment

    test::JoiningThread releaser([&] {
        this_thread::sleep_for(chrono::milliseconds(100));
        msgs.clear();
    });
    auto const start = chrono::steady_clock::now();
    MessagePtr msg;
    ASSERT_NO_THROW(msg = factory->CreateMessage(kMsgSize));
    EXPECT_NE(msg, nullptr);
    EXPECT_GE(chrono::steady_clock::now() - start, chrono::milliseconds(50));
}

TEST_P(ShmAllocation, InterruptStopsTheWait)
{
    fConfig.SetProperty<bool>("shm-throw-bad-alloc", false);
    fConfig.SetProperty<int>("bad-alloc-attempt-interval", 5);
    auto factory = Factory();
    vector<MessagePtr> msgs;
    for (size_t i = 0; i < kSegmentSize / kMsgSize - 1; ++i) {
        msgs.push_back(factory->CreateMessage(kMsgSize));
    }
    auto const size = kSegmentSize / 2; // larger than the free memory, smaller than the segment

    test::JoiningThread interrupter([&] {
        this_thread::sleep_for(chrono::milliseconds(50));
        factory->Interrupt();
    });
    EXPECT_THROW(factory->CreateMessage(size), MessageBadAlloc);
}

INSTANTIATE_TEST_SUITE_P(, ShmAllocation, testing::Values("rbtree_best_fit", "simple_seq_fit"),
                         [](testing::TestParamInfo<string> const& p) { return p.param; });

} // namespace
