/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIRMQSTATEMACHINE_H_
#define FAIRMQSTATEMACHINE_H_

#include "FairMQLogger.h"

#include <string>
#include <memory>
#include <functional>
#include <condition_variable>
#include <ostream>
#include <queue>
#include <mutex>
#include <stdexcept>

namespace fair
{
namespace mq
{

enum class State : int
{
    Ok,
    Error,
    Idle,
    InitializingDevice,
    Initialized,
    Binding,
    Bound,
    Connecting,
    DeviceReady,
    InitializingTask,
    Ready,
    Running,
    ResettingTask,
    ResettingDevice,
    Exiting
};

enum class Transition : int
{
    Auto,
    InitDevice,
    CompleteInit,
    Bind,
    Connect,
    InitTask,
    Run,
    Stop,
    ResetTask,
    ResetDevice,
    End,
    ErrorFound
};

class StateMachine
{
  public:
    StateMachine();
    virtual ~StateMachine();

    bool ChangeState(const Transition transition);
    bool ChangeState(const std::string& transition) { return ChangeState(GetTransition(transition)); }

    void SubscribeToStateChange(const std::string& key, std::function<void(const State)> callback);
    void UnsubscribeFromStateChange(const std::string& key);

    void HandleStates(std::function<void(const State)> callback);
    void StopHandlingStates();

    void SubscribeToNewTransition(const std::string& key, std::function<void(const Transition)> callback);
    void UnsubscribeFromNewTransition(const std::string& key);

    bool NewStatePending() const;
    void WaitForPendingState() const;
    bool WaitForPendingStateFor(const int durationInMs) const;

    State GetCurrentState() const;
    std::string GetCurrentStateName() const;

    void Start();

    void ProcessWork();

    static std::string GetStateName(const State);
    static std::string GetTransitionName(const Transition);
    static State GetState(const std::string& state);
    static Transition GetTransition(const std::string& transition);

  private:
    std::shared_ptr<void> fFsm;
};

inline std::ostream& operator<<(std::ostream& os, const State& state) { return os << StateMachine::GetStateName(state); }
inline std::ostream& operator<<(std::ostream& os, const Transition& transition) { return os << StateMachine::GetTransitionName(transition); }

} // namespace mq
} // namespace fair

#endif /* FAIRMQSTATEMACHINE_H_ */
