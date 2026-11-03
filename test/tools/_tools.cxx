/********************************************************************************
 *    Copyright (C) 2026 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <fairmq/tools/InstanceLimit.h>
#include <fairmq/tools/Process.h>
#include <fairmq/tools/RateLimit.h>
#include <fairmq/tools/Semaphore.h>
#include <fairmq/tools/Strings.h>
#include <fairmq/tools/Unique.h>

#include <helper/Sync.h>

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <csignal>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{

using namespace std;
using namespace fair::mq::tools;

TEST(RateLimiter, LimitsTheRate)
{
    // 100 calls at 200 Hz take at least approx. 0.5 s. Check only a loose
    // lower bound: a slow machine can make the loop slower, never faster.
    RateLimiter limiter(200);
    auto const start = chrono::steady_clock::now();
    for (int i = 0; i < 100; ++i) {
        limiter.maybe_sleep();
    }
    auto const elapsed = chrono::steady_clock::now() - start;
    EXPECT_GE(elapsed, chrono::milliseconds(350));
    EXPECT_LT(elapsed, chrono::seconds(5));
}

TEST(RateLimiter, ZeroRateDoesNotSleep)
{
    RateLimiter limiter(0);
    auto const start = chrono::steady_clock::now();
    for (int i = 0; i < 100000; ++i) {
        limiter.maybe_sleep();
    }
    EXPECT_LT(chrono::steady_clock::now() - start, chrono::seconds(1));
}

struct TestTag
{
    friend auto operator<<(ostream& os, TestTag) -> ostream& { return os << "TestTag"; }
};

TEST(InstanceLimiter, ThrowsAboveTheLimit)
{
    using Limiter = InstanceLimiter<TestTag, 2>;
    {
        Limiter first;
        Limiter second;
        EXPECT_EQ(first.GetCount(), 2);
        try {
            Limiter third;
            ADD_FAILURE() << "no exception for the third instance";
        } catch (runtime_error const& e) {
            EXPECT_EQ(string(e.what()), "More than 2 instances of TestTag in parallel not supported");
        }
        EXPECT_EQ(first.GetCount(), 2);
    }
    Limiter again;
    EXPECT_EQ(again.GetCount(), 1);
}

TEST(Semaphore, CountsSignalsAndWaits)
{
    Semaphore sem(2);
    EXPECT_EQ(sem.GetCount(), 2U);
    sem.Wait();
    sem.Wait();
    EXPECT_EQ(sem.GetCount(), 0U);
    sem.Signal();
    EXPECT_EQ(sem.GetCount(), 1U);
    EXPECT_EQ(Semaphore().GetCount(), 0U);
}

TEST(Semaphore, WaitBlocksUntilSignal)
{
    Semaphore sem;
    atomic<bool> done{false};
    fair::mq::test::JoiningThread waiter([&] {
        sem.Wait();
        done = true;
    });
    this_thread::sleep_for(chrono::milliseconds(20));
    EXPECT_FALSE(done);
    sem.Signal();
    EXPECT_TRUE(fair::mq::test::WaitUntil([&] { return done.load(); }));
}

TEST(SharedSemaphore, CopiesShareTheCount)
{
    SharedSemaphore sem(1);
    SharedSemaphore copy = sem; // NOLINT(performance-unnecessary-copy-initialization)
    copy.Signal();
    EXPECT_EQ(sem.GetCount(), 2U);
    sem.Wait();
    EXPECT_EQ(copy.GetCount(), 1U);
}

TEST(Strings, ToString)
{
    EXPECT_EQ(ToString(), "");
    EXPECT_EQ(ToString("a", 1, '-', 2.5, string("z")), "a1-2.5z");
}

TEST(Strings, StartsAndEndsWith)
{
    EXPECT_TRUE(StrStartsWith("fairmq", "fair"));
    EXPECT_TRUE(StrStartsWith("fairmq", ""));
    EXPECT_FALSE(StrStartsWith("fair", "fairmq"));
    EXPECT_FALSE(StrStartsWith("fairmq", "mq"));
    EXPECT_TRUE(StrEndsWith("fairmq", "mq"));
    EXPECT_TRUE(StrEndsWith("fairmq", ""));
    EXPECT_FALSE(StrEndsWith("mq", "fairmq"));
    EXPECT_FALSE(StrEndsWith("fairmq", "fair"));
}

TEST(Strings, ToStrVector)
{
    char prog[] = "prog";
    char a[] = "--a";
    char b[] = "b";
    char* argv[] = {prog, a, b};
    EXPECT_EQ(ToStrVector(3, argv), (vector<string>{"--a", "b"}));
    EXPECT_EQ(ToStrVector(3, argv, false), (vector<string>{"prog", "--a", "b"}));
}

TEST(Unique, UuidsAreUnique)
{
    set<string> uuids;
    set<size_t> hashes;
    for (int i = 0; i < 1000; ++i) {
        auto const uuid = Uuid();
        EXPECT_EQ(uuid.size(), 36U) << uuid;
        uuids.insert(uuid);
        hashes.insert(UuidHash());
    }
    EXPECT_EQ(uuids.size(), 1000U);
    EXPECT_EQ(hashes.size(), 1000U);
}

TEST(Execute, OutputAndExitCode)
{
    auto const result = execute(R"(sh -c "echo out; echo err 1>&2; exit 3")", "[p] ");
    EXPECT_EQ(result.exit_code, 3);
    EXPECT_NE(result.console_out.find("[p] out\n"), string::npos) << result.console_out;
    EXPECT_NE(result.console_out.find("[p] error: err\n"), string::npos) << result.console_out;
}

TEST(Execute, Input)
{
    auto const result = execute("head -n 1", "", "from stdin\n");
    EXPECT_EQ(result.exit_code, 0);
    EXPECT_NE(result.console_out.find("from stdin"), string::npos) << result.console_out;
}

TEST(Execute, Signal)
{
    // execute() sends the signal approx. 2 s after the start.
    auto const result = execute("sleep 30", "", "", SIGTERM);
    EXPECT_EQ(result.exit_code, SIGTERM);
}

TEST(Execute, Timeout)
{
    auto const start = chrono::steady_clock::now();
    auto const result = execute("sleep 30", "", "", -1, chrono::milliseconds(200));
    EXPECT_LT(chrono::steady_clock::now() - start, chrono::seconds(10));
    EXPECT_EQ(result.exit_code, SIGKILL);
    EXPECT_NE(result.console_out.find("timeout after 200 ms"), string::npos) << result.console_out;
}

} // namespace
