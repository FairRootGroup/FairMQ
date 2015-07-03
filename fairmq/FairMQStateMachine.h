/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *
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

#define FAIRMQ_INTERFACE_VERSION 2

#include <string>

#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>

#include <boost/msm/back/state_machine.hpp>
#include <boost/msm/front/state_machine_def.hpp>
#include <boost/msm/front/functor_row.hpp>

#include "FairMQLogger.h"

namespace msm = boost::msm;
namespace mpl = boost::mpl;
namespace msmf = boost::msm::front;

namespace FairMQFSM
{

// defining events for the boost MSM state machine
struct INIT_DEVICE           { std::string name() const { return "INIT_DEVICE"; } };
struct internal_DEVICE_READY { std::string name() const { return "internal_DEVICE_READY"; } };
struct INIT_TASK             { std::string name() const { return "INIT_TASK"; } };
struct internal_READY        { std::string name() const { return "internal_READY"; } };
struct RUN                   { std::string name() const { return "RUN"; } };
struct PAUSE                 { std::string name() const { return "PAUSE"; } };
struct STOP                  { std::string name() const { return "STOP"; } };
struct RESET_TASK            { std::string name() const { return "RESET_TASK"; } };
struct RESET_DEVICE          { std::string name() const { return "RESET_DEVICE"; } };
struct internal_IDLE         { std::string name() const { return "internal_IDLE"; } };
struct END                   { std::string name() const { return "END"; } };

// defining the boost MSM state machine
struct FairMQFSM_ : public msm::front::state_machine_def<FairMQFSM_>
{
  public:
    FairMQFSM_()
        : fState()
        , fStateThread()
        , fTerminateStateThread()
        , fStateFinished(false)
        , fStateCondition()
        , fStateMutex()
        {}

    // Destructor
    virtual ~FairMQFSM_() {};

    template <class Event, class FSM>
    void on_entry(Event const&, FSM&)
    {
        LOG(STATE) << "Entering FairMQ state machine";
        fState = IDLE;
    }

    template <class Event, class FSM>
    void on_exit(Event const&, FSM&)
    {
        LOG(STATE) << "Exiting FairMQ state machine";
    }

    // The list of FSM states
    struct IDLE_FSM : public msm::front::state<> {};
    struct INITIALIZING_DEVICE_FSM : public msm::front::state<> {};
    struct DEVICE_READY_FSM : public msm::front::state<> {};
    struct INITIALIZING_TASK_FSM : public msm::front::state<> {};
    struct READY_FSM : public msm::front::state<> {};
    struct RUNNING_FSM : public msm::front::state<> {};
    struct PAUSED_FSM : public msm::front::state<> {};
    struct RESETTING_TASK_FSM : public msm::front::state<> {};
    struct RESETTING_DEVICE_FSM : public msm::front::state<> {};
    struct EXITING_FSM : public msm::front::state<> {};

    // Define initial state
    typedef IDLE_FSM initial_state;

    // Actions
    struct IdleFct
    {
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(EVT const&, FSM& fsm, SourceState&, TargetState&)
        {
            fsm.fState = IDLE;
            LOG(STATE) << "Entering IDLE state";
        }
    };

    struct InitDeviceFct
    {
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(EVT const&, FSM& fsm, SourceState&, TargetState&)
        {
            fsm.fStateFinished = false;
            LOG(STATE) << "Entering INITIALIZING DEVICE state";
            fsm.fState = INITIALIZING_DEVICE;
            fsm.fStateThread = boost::thread(boost::bind(&FairMQFSM_::InitWrapper, &fsm));
        }
    };

    struct DeviceReadyFct
    {
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(EVT const&, FSM& fsm, SourceState&, TargetState&)
        {
            LOG(STATE) << "Entering DEVICE READY state";
            fsm.fState = DEVICE_READY;
        }
    };

    struct InitTaskFct
    {
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(EVT const&, FSM& fsm, SourceState&, TargetState&)
        {
            fsm.fStateFinished = false;
            LOG(STATE) << "Entering INITIALIZING TASK state";
            fsm.fState = INITIALIZING_TASK;
            fsm.InitTaskWrapper();
            // fsm.fInitializingTaskThread = boost::thread(boost::bind(&FairMQFSM_::InitTaskWrapper, &fsm));
        }
    };

    struct ReadyFct
    {
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(EVT const&, FSM& fsm, SourceState&, TargetState&)
        {
            LOG(STATE) << "Entering READY state";
            fsm.fState = READY;
        }
    };

    struct RunFct
    {
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(EVT const&, FSM& fsm, SourceState&, TargetState&)
        {
            fsm.fStateFinished = false;
            LOG(STATE) << "Entering RUNNING state";
            fsm.fState = RUNNING;
            fsm.fStateThread = boost::thread(boost::bind(&FairMQFSM_::RunWrapper, &fsm));
        }
    };

    struct PauseFct
    {
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(EVT const&, FSM& fsm, SourceState&, TargetState&)
        {
            fsm.fStateFinished = false;
            fsm.fState = PAUSED;
            fsm.SendCommand("pause");
            fsm.fStateThread.join();
            LOG(STATE) << "Entering PAUSED state";
            fsm.fStateThread = boost::thread(boost::bind(&FairMQFSM_::Pause, &fsm));
        }
    };

    struct ResumeFct
    {
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(EVT const&, FSM& fsm, SourceState&, TargetState&)
        {
            fsm.fState = RUNNING;
            fsm.fStateThread.interrupt();
            fsm.fStateThread.join();
            fsm.fStateFinished = false;
            LOG(STATE) << "Entering RUNNING state";
            fsm.fStateThread = boost::thread(boost::bind(&FairMQFSM_::RunWrapper, &fsm));
        }
    };

    struct StopFct
    {
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(EVT const&, FSM& fsm, SourceState&, TargetState&)
        {
            LOG(STATE) << "Received STOP event";
            fsm.fState = IDLE;
            fsm.SendCommand("stop");
            fsm.fStateThread.join();
        }
    };

    struct InternalStopFct
    {
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(EVT const&, FSM& fsm, SourceState&, TargetState&)
        {
            LOG(STATE) << "RUNNING state finished without an external event";
            fsm.fState = IDLE;
        }
    };

    struct ResetTaskFct
    {
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(EVT const&, FSM& fsm, SourceState&, TargetState&)
        {
            fsm.fStateFinished = false;
            LOG(STATE) << "Entering RESETTING TASK state";
            fsm.fState = RESETTING_TASK;
            fsm.fStateThread = boost::thread(boost::bind(&FairMQFSM_::ResetTaskWrapper, &fsm));
        }
    };

    struct ResetDeviceFct
    {
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(EVT const&, FSM& fsm, SourceState&, TargetState&)
        {
            fsm.fStateFinished = false;
            LOG(STATE) << "Entering RESETTING DEVICE state";
            fsm.fState = RESETTING_DEVICE;
            fsm.ResetWrapper();
        }
    };

    struct ExitingFct
    {
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(EVT const&, FSM& fsm, SourceState&, TargetState&)
        {
            LOG(STATE) << "Received END event";
            fsm.fState = EXITING;

            fsm.fTerminateStateThread = boost::thread(boost::bind(&FairMQFSM_::Terminate, &fsm));
            fsm.Shutdown();
            fsm.fTerminateStateThread.join();
        }
    };

    struct ExitingRunFct
    {
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(EVT const&, FSM& fsm, SourceState&, TargetState&)
        {
            LOG(STATE) << "Entering EXITING state";
            fsm.fState = EXITING;
            fsm.SendCommand("stop");
            fsm.fStateThread.interrupt();
            fsm.fStateThread.join();

            fsm.fTerminateStateThread = boost::thread(boost::bind(&FairMQFSM_::Terminate, &fsm));
            fsm.Shutdown();
            fsm.fTerminateStateThread.join();
        }
    };

    // actions to be overwritten by derived classes
    virtual void InitWrapper() {}
    virtual void Init() {}
    virtual void InitTaskWrapper() {}
    virtual void InitTask() {}
    virtual void RunWrapper() {}
    virtual void Run() {}
    virtual void Pause() {}
    virtual void ResetWrapper() {}
    virtual void Reset() {}
    virtual void ResetTaskWrapper() {}
    virtual void ResetTask() {}
    virtual void Shutdown() {}
    virtual void Terminate() {} // Termination method called during StopFct action.
    virtual void SendCommand(const std::string& command) {} // Method to send commands.

    // Transition table for FairMQFMS
    struct transition_table : mpl::vector<
        //        Start                    Event                  Next                     Action          Guard
        //       +-------------------------+----------------------+------------------------+---------------+---------+
        msmf::Row<IDLE_FSM,                INIT_DEVICE,           INITIALIZING_DEVICE_FSM, InitDeviceFct,   msmf::none>,
        msmf::Row<INITIALIZING_DEVICE_FSM, internal_DEVICE_READY, DEVICE_READY_FSM,        DeviceReadyFct,  msmf::none>,
        msmf::Row<DEVICE_READY_FSM,        INIT_TASK,             INITIALIZING_TASK_FSM,   InitTaskFct,     msmf::none>,
        msmf::Row<INITIALIZING_TASK_FSM,   internal_READY,        READY_FSM,               ReadyFct,        msmf::none>,
        msmf::Row<READY_FSM,               RUN,                   RUNNING_FSM,             RunFct,          msmf::none>,
        msmf::Row<RUNNING_FSM,             PAUSE,                 PAUSED_FSM,              PauseFct,        msmf::none>,
        msmf::Row<PAUSED_FSM,              RUN,                   RUNNING_FSM,             ResumeFct,       msmf::none>,
        msmf::Row<RUNNING_FSM,             STOP,                  READY_FSM,               StopFct,         msmf::none>,
        msmf::Row<RUNNING_FSM,             internal_READY,        READY_FSM,               InternalStopFct, msmf::none>,
        msmf::Row<READY_FSM,               RESET_TASK,            RESETTING_TASK_FSM,      ResetTaskFct,    msmf::none>,
        msmf::Row<RESETTING_TASK_FSM,      internal_DEVICE_READY, DEVICE_READY_FSM,        DeviceReadyFct,  msmf::none>,
        msmf::Row<DEVICE_READY_FSM,        RESET_DEVICE,          RESETTING_DEVICE_FSM,    ResetDeviceFct,  msmf::none>,
        msmf::Row<RESETTING_DEVICE_FSM,    internal_IDLE,         IDLE_FSM,                IdleFct,         msmf::none>,
        msmf::Row<RUNNING_FSM,             END,                   EXITING_FSM,             ExitingRunFct,   msmf::none>, // temporary
        msmf::Row<IDLE_FSM,                END,                   EXITING_FSM,             ExitingFct,      msmf::none> >
        {};

    // Replaces the default no-transition response.
    template <class FSM, class Event>
    void no_transition(Event const& e, FSM&, int state)
    {
        LOG(STATE) << "no transition from state " << GetStateName(state) << " on event " << e.name();
    }

    // this is to run certain functions in a separate thread
    boost::thread fStateThread;
    boost::thread fTerminateStateThread;

    // backward compatibility to FairMQStateMachine
    enum State
    {
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

    std::string GetStateName(int state) const
    {
        switch(state)
        {
            case IDLE:
                return "IDLE";
            case INITIALIZING_DEVICE:
                return "INITIALIZING_DEVICE";
            case DEVICE_READY:
                return "DEVICE_READY";
            case INITIALIZING_TASK:
                return "INITIALIZING_TASK";
            case READY:
                return "READY";
            case RUNNING:
                return "RUNNING";
            case PAUSED:
                return "PAUSED";
            case RESETTING_TASK:
                return "RESETTING_TASK";
            case RESETTING_DEVICE:
                return "RESETTING_DEVICE";
            case EXITING:
                return "EXITING";
            default:
                return "requested name for non-existent state...";
        }
    }

    std::string GetCurrentStateName() const
    {
        return GetStateName(fState);
    }

    int GetCurrentState() const
    {
        return fState;
    }

    bool CheckCurrentState(int state) const
    {
        if (state == fState)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    bool CheckCurrentState(std::string state) const
    {
        if (state == GetCurrentStateName())
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    // condition variable to notify parent thread about end of state.
    bool fStateFinished;
    boost::condition_variable fStateCondition;
    boost::mutex fStateMutex;

  private:
    State fState;
};

typedef msm::back::state_machine<FairMQFSM_> FairMQFSM;
} // namespace FairMQFSM

class FairMQStateMachine : public FairMQFSM::FairMQFSM
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
        END
    };
    FairMQStateMachine();
    virtual ~FairMQStateMachine();

    int GetInterfaceVersion();

    int GetEventNumber(std::string event);

    bool ChangeState(int event);
    bool ChangeState(std::string event);

    void WaitForEndOfState(int state);
    void WaitForEndOfState(std::string state);

    bool WaitForEndOfStateForMs(int state, int durationInMs);
    bool WaitForEndOfStateForMs(std::string state, int durationInMs);
};

#endif /* FAIRMQSTATEMACHINE_H_ */
