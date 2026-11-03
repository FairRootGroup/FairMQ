/********************************************************************************
 *    Copyright (C) 2026 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_TEST_SYNC_H
#define FAIR_MQ_TEST_SYNC_H

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <thread>
#include <utility>

namespace fair::mq::test
{

/// Default deadline for a test to wait for an event. Long enough for slow
/// (e.g. sanitizer) builds, short enough to fail well before the ctest timeout.
constexpr auto kDefaultTimeout = std::chrono::seconds(10);

/// Poll pred until it returns true or the timeout expires.
/// Returns the last result of pred.
template<typename Pred, typename Rep, typename Period>
bool WaitUntil(Pred&& pred, std::chrono::duration<Rep, Period> timeout)
{
    auto const deadline = std::chrono::steady_clock::now() + timeout;
    while (!pred()) {
        if (std::chrono::steady_clock::now() >= deadline) {
            return pred();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return true;
}

template<typename Pred>
bool WaitUntil(Pred&& pred)
{
    return WaitUntil(std::forward<Pred>(pred), kDefaultTimeout);
}

/// Counting semaphore with a deadline on Wait().
/// Use it to wait for callbacks: a failing check in the callback must not
/// turn into a hang of the test.
class Semaphore
{
  public:
    void Signal()
    {
        {
            std::lock_guard<std::mutex> lock(fMtx);
            ++fCount;
        }
        fCv.notify_one();
    }

    /// Returns false if the timeout expires before a Signal().
    template<typename Rep, typename Period>
    [[nodiscard]] bool Wait(std::chrono::duration<Rep, Period> timeout)
    {
        std::unique_lock<std::mutex> lock(fMtx);
        if (!fCv.wait_for(lock, timeout, [this] { return fCount > 0; })) {
            return false;
        }
        --fCount;
        return true;
    }

    [[nodiscard]] bool Wait() { return Wait(kDefaultTimeout); }

  private:
    std::mutex fMtx;
    std::condition_variable fCv;
    std::size_t fCount = 0;
};

/// std::thread that joins on destruction (C++17 has no std::jthread).
/// An early return from a test (e.g. a failed ASSERT_*) then does not call
/// std::terminate().
class JoiningThread
{
  public:
    template<typename F, typename... Args>
    explicit JoiningThread(F&& f, Args&&... args)
        : fThread(std::forward<F>(f), std::forward<Args>(args)...)
    {}

    JoiningThread(const JoiningThread&) = delete;
    JoiningThread(JoiningThread&&) = delete;
    JoiningThread& operator=(const JoiningThread&) = delete;
    JoiningThread& operator=(JoiningThread&&) = delete;

    ~JoiningThread() { Join(); }

    void Join()
    {
        if (fThread.joinable()) {
            fThread.join();
        }
    }

  private:
    std::thread fThread;
};

} // namespace fair::mq::test

#endif /* FAIR_MQ_TEST_SYNC_H */
