/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIRMQSTATEMACHINE_H_
#define FAIRMQSTATEMACHINE_H_

#include <fairmq/States.h>

#include <fairlogger/Logger.h>

#include <string>
#include <memory>
#include <functional>
#include <stdexcept>

namespace fair
{
namespace mq
{

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

    struct ErrorStateException : std::runtime_error { using std::runtime_error::runtime_error; };

  private:
    std::shared_ptr<void> fFsm;
};

} // namespace mq
} // namespace fair

#endif /* FAIRMQSTATEMACHINE_H_ */
