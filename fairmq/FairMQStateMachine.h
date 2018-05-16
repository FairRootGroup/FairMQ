/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQStateMachine.h
 *
 * @since 2012-10-25
 * @author D. Klein, A. Rybalchenko
 */

#ifndef FAIRMQSTATEMACHINE_H_
#define FAIRMQSTATEMACHINE_H_

#define FAIRMQ_INTERFACE_VERSION 3

#include "FairMQLogger.h"

#include <string>
#include <memory>
#include <functional>
#include <mutex>

class FairMQStateMachine
{
  public:
    enum Event
    {
        INIT_DEVICE,
        internal_DEVICE_READY,
        INIT_TASK,
        internal_READY,
        RUN,
        PAUSE,
        STOP,
        RESET_TASK,
        RESET_DEVICE,
        internal_IDLE,
        END,
        ERROR_FOUND
    };

    enum State
    {
        OK,
        Error,
        IDLE,
        INITIALIZING_DEVICE,
        DEVICE_READY,
        INITIALIZING_TASK,
        READY,
        RUNNING,
        PAUSED,
        RESETTING_TASK,
        RESETTING_DEVICE,
        EXITING
    };

    FairMQStateMachine();
    virtual ~FairMQStateMachine();

    int GetInterfaceVersion() const;

    bool ChangeState(int event);
    bool ChangeState(const std::string& event);

    void WaitForEndOfState(int event);
    void WaitForEndOfState(const std::string& event);

    bool WaitForEndOfStateForMs(int event, int durationInMs);
    bool WaitForEndOfStateForMs(const std::string& event, int durationInMs);

    void SubscribeToStateChange(const std::string& key, std::function<void(const State)> callback);
    void UnsubscribeFromStateChange(const std::string& key);

    void CallStateChangeCallbacks(const State state) const;

    std::string GetCurrentStateName() const;
    int GetCurrentState() const;
    bool CheckCurrentState(int state) const;
    bool CheckCurrentState(std::string state) const;
    bool Terminated();

    // actions to be overwritten by derived classes
    virtual void InitWrapper() {}
    virtual void InitTaskWrapper() {}
    virtual void RunWrapper() {}
    virtual void PauseWrapper() {}
    virtual void ResetWrapper() {}
    virtual void ResetTaskWrapper() {}
    virtual void Exit() {}
    virtual void Unblock() {}

  private:
    int GetEventNumber(const std::string& event);

    std::mutex fChangeStateMutex;

    std::shared_ptr<void> fFsm;
};

#endif /* FAIRMQSTATEMACHINE_H_ */
