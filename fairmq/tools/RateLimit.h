/********************************************************************************
 * Copyright (C) 2014-2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_TOOLS_RATELIMIT_H
#define FAIR_MQ_TOOLS_RATELIMIT_H

#include <cassert>
#include <string>
// #include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>

namespace fair::mq::tools
{

/**
 * Objects of type RateLimiter can be used to limit a loop to a given rate of iterations per second.
 *
 * Example:
 * \code
 * RateLimiter limit(100);  // 100 Hz
 * while (do_more_work()) {
 *   work();
 *   limit.maybe_sleep();   // this needs to be at the end of the loop for a
 *                          // correct time measurement of the first iterations
 * }
 * \endcode
 */
class RateLimiter
{
    using clock = std::chrono::steady_clock;

  public:
    /**
     * Constructs a rate limiter.
     *
     * \param rate Work rate in Hz (calls to maybe_sleep per second). Values less than/equal
     *             to 0 set the rate to 1 GHz (which is impossible to achieve, even with a
     *             loop that only calls RateLimiter::maybe_sleep).
     */
    explicit RateLimiter(float rate)
        : tw_req(std::chrono::seconds(1))
        , start_time(clock::now())
    {
        if (rate <= 0) {
            tw_req = std::chrono::nanoseconds(1);
        } else {
            tw_req = std::chrono::duration_cast<clock::duration>(tw_req / rate);
        }
        skip_check_count = std::max(1, int(std::chrono::milliseconds(5) / tw_req));
        count = skip_check_count;
        // std::cerr << "skip_check_count: " << skip_check_count << '\n';
    }

    /**
     * Call this function at the end of the iteration rate limited loop.
     *
     * This function might use `std::this_thread::sleep_for` to limit the iteration rate. If no
     * sleeps are necessary, the function will back off checking for the time to further allow
     * increased iteration rates (until the requested rate or 1s between rechecks is reached).
     */
    void maybe_sleep()
    {
        using namespace std::chrono;
        if (--count == 0) {
            auto now = clock::now();
            if (tw == clock::duration::zero()) {
                tw = (now - start_time) / skip_check_count;
            } else {
                tw = (1 * tw + 3 * (now - start_time) / skip_check_count) / 4;
            }
            // std::ostringstream s; s << "tw = " << std::setw(10) <<
            // duration_cast<nanoseconds>(tw).count() << "ns, req = " <<
            // duration_cast<nanoseconds>(tw_req).count() << "ns, ";
            if (tw > tw_req * 65 / 64) {
                // the time between maybe_sleep calls is more than 1% too long
                // fix it by reducing ts towards 0 and if ts = 0 doesn't suffice, increase
                // skip_check_count
                if (ts > clock::duration::zero()) {
                    ts = std::max(clock::duration::zero(), ts - (tw - tw_req) * skip_check_count * 1 / 2);
                    // std::cerr << s.str() << "maybe_sleep: going too slow; sleep less: " <<
                    // duration_cast<microseconds>(ts).count() << "µs\n";
                } else {
                    skip_check_count =
                        std::min(int(seconds(1) / tw_req),   // recheck at least every second
                                 (skip_check_count * 5 + 3) / 4);
                    // std::cerr << s.str() << "maybe_sleep: going too slow; work more: " <<
                    // skip_check_count << "\n";
                }
            } else if (tw < tw_req * 63 / 64) {
                // the time between maybe_sleep calls is more than 1% too short
                // fix it by reducing skip_check_count towards 1 and if skip_check_count = 1
                // doesn't suffice, increase ts

                // The minimum work count is defined such that a typical sleep time is greater
                // than 1ms.
                // The user requested 1/tw_req work iterations per second. Divided by 1000, that's
                // the count per ms.
                const int min_skip_count = std::max(1, int(milliseconds(5) / tw_req));
                if (skip_check_count > min_skip_count) {
                    assert(ts == clock::duration::zero());
                    skip_check_count = std::max(min_skip_count, skip_check_count * 3 / 4);
                    // std::cerr << s.str() << "maybe_sleep: going too fast; work less: " <<
                    // skip_check_count << "\n";
                } else {
                    ts += (tw_req - tw) * (skip_check_count * 7) / 8;
                    // std::cerr << s.str() << "maybe_sleep: going too fast; sleep more: " <<
                    // duration_cast<microseconds>(ts).count() << "µs\n";
                }
            }

            start_time = now;
            count = skip_check_count;
            if (ts > clock::duration::zero()) {
                std::this_thread::sleep_for(ts);
            }
        }
    }

  private:
    clock::duration tw{},   //! deduced duration between maybe_sleep calls
        ts{},               //! sleep duration
        tw_req;             //! requested duration between maybe_sleep calls
    clock::time_point start_time;
    int count = 1;
    int skip_check_count = 1;
};

} // namespace fair::mq::tools

#endif  // FAIR_MQ_TOOLS_RATELIMIT_H
