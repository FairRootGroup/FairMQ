/********************************************************************************
 *    Copyright (C) 2019 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIRMQSTATEQUEUE_H_
#define FAIRMQSTATEQUEUE_H_

#include <fairmq/States.h>

#include <queue>
#include <mutex>
#include <chrono>
#include <utility> // pair
#include <condition_variable>

namespace fair
{
namespace mq
{

class StateQueue
{
  public:
    StateQueue() {}
    ~StateQueue() {}

    fair::mq::State WaitForNext()
    {
        std::unique_lock<std::mutex> lock(fMtx);
        while (fStates.empty()) {
            fCV.wait_for(lock, std::chrono::milliseconds(50));
        }

        fair::mq::State state = fStates.front();

        if (state == fair::mq::State::Error) {
            throw DeviceErrorState("Controlled device transitioned to error state.");
        }

        fStates.pop();
        return state;
    }

    template<typename Rep, typename Period>
    std::pair<bool, fair::mq::State> WaitForNext(std::chrono::duration<Rep, Period> const& duration)
    {
        std::unique_lock<std::mutex> lock(fMtx);
        fCV.wait_for(lock, duration);

        if (fStates.empty()) {
            return { false, fair::mq::State::Ok };
        }

        fair::mq::State state = fStates.front();

        if (state == fair::mq::State::Error) {
            throw DeviceErrorState("Controlled device transitioned to error state.");
        }

        fStates.pop();
        return { true, state };
    }

    void WaitForState(fair::mq::State state) { while (WaitForNext() != state) {} }

    void Push(fair::mq::State state)
    {
        {
            std::lock_guard<std::mutex> lock(fMtx);
            fStates.push(state);
        }
        fCV.notify_all();
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
};

} // namespace mq
} // namespace fair

#endif /* FAIRMQSTATEQUEUE_H_ */