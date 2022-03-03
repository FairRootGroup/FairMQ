/********************************************************************************
 * Copyright (C) 2019-2022 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIRMQSTATEQUEUE_H_
#define FAIRMQSTATEQUEUE_H_

#include <chrono>
#include <condition_variable>
#include <fairmq/States.h>
#include <mutex>
#include <queue>
#include <utility>   // pair

namespace fair::mq {

class StateQueue
{
  public:
    StateQueue() = default;
    StateQueue(const StateQueue&) = delete;
    StateQueue(StateQueue&&) = delete;
    StateQueue& operator=(const StateQueue&) = delete;
    StateQueue& operator=(StateQueue&&) = delete;
    ~StateQueue() = default;

    fair::mq::State WaitForNext()
    {
        std::unique_lock<std::mutex> lock(fMtx);
        fCV.wait(lock, [this] { return Predicate(); });
        return PopFrontUnsafe();
    }

    template<typename Timeout>
    std::pair<bool, fair::mq::State> WaitForNext(Timeout&& duration)
    {
        std::unique_lock<std::mutex> lock(fMtx);
        fCV.wait_for(lock, std::forward<Timeout>(duration), [this] { return Predicate(); });
        return ReturnPairUnsafe();
    }

    template<typename CustomPredicate>
    std::pair<bool, fair::mq::State> WaitForNextOrCustom(CustomPredicate&& customPredicate)
    {
        std::unique_lock<std::mutex> lock(fMtx);
        fCV.wait(lock, [this, cp = std::move(customPredicate)] { return Predicate() || cp(); });
        return ReturnPairUnsafe();
    }

    template<typename CustomPredicate>
    std::pair<bool, fair::mq::State> WaitForCustom(CustomPredicate&& customPredicate)
    {
        std::unique_lock<std::mutex> lock(fMtx);
        fCV.wait(lock, [cp = std::move(customPredicate)] { return cp(); });
        return ReturnPairUnsafe();
    }

    void WaitForState(fair::mq::State state)
    {
        while (WaitForNext() != state) {}
    }

    template<typename CustomPredicate>
    void WaitForStateOrCustom(fair::mq::State state, CustomPredicate customPredicate)
    {
        auto next = WaitForNextOrCustom(customPredicate);
        while (!customPredicate() && (next.first && next.second != state)) {
            next = WaitForNextOrCustom(customPredicate);
        }
    }

    void Push(fair::mq::State state)
    {
        {
            std::lock_guard<std::mutex> lock(fMtx);
            fStates.push(state);
        }
        fCV.notify_one();
    }

    template<typename CustomSignal>
    void Push(fair::mq::State state, CustomSignal&& signal)
    {
        {
            std::lock_guard<std::mutex> lock(fMtx);
            fStates.push(state);
            signal();
        }
        fCV.notify_one();
    }

    template<typename CustomSignal>
    void Notify(CustomSignal&& signal)
    {
        {
            std::lock_guard<std::mutex> lock(fMtx);
            signal();
        }
        fCV.notify_one();
    }

    template<typename CustomSignal>
    void Locked(CustomSignal&& signal)
    {
        std::lock_guard<std::mutex> lock(fMtx);
        signal();
    }

    void Clear()
    {
        std::lock_guard<std::mutex> lock(fMtx);
        fStates = std::queue<fair::mq::State>();
    }

  private:
    std::queue<fair::mq::State> fStates;
    std::mutex fMtx;
    std::condition_variable fCV;

    // must be called under locked fMtx
    fair::mq::State PopFrontUnsafe()
    {
        fair::mq::State state = fStates.front();
        if (state == fair::mq::State::Error) {
            throw DeviceErrorState("Controlled device transitioned to error state.");
        }
        fStates.pop();
        return state;
    }

    // must be called under locked fMtx
    std::pair<bool, fair::mq::State> ReturnPairUnsafe()
    {
        auto const pred = Predicate();
        return {pred, pred ? PopFrontUnsafe() : fair::mq::State::Ok};
    }

    // must be called under locked fMtx
    bool Predicate() { return !fStates.empty(); }
};

}   // namespace fair::mq

#endif /* FAIRMQSTATEQUEUE_H_ */
