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

#define FAIRMQ_INTERFACE_VERSION 3

#include <string>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <functional>
#include <unordered_map>

// Increase maximum number of boost::msm states (default is 10)
// This #define has to be before any msm header includes
#define FUSION_MAX_VECTOR_SIZE 20

#include <boost/mpl/for_each.hpp>
#include <boost/msm/back/state_machine.hpp>
#include <boost/msm/back/tools.hpp>
#include <boost/msm/back/metafunctions.hpp>
#include <boost/msm/front/state_machine_def.hpp>
#include <boost/msm/front/functor_row.hpp>

#include <boost/signals2.hpp> // signal/slot for onStateChange callbacks

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
struct ERROR_FOUND           { std::string name() const { return "ERROR_FOUND"; } };

// deactivate the warning for non-virtual destructor thrown in the boost library
#if defined(__clang__)
_Pragma("clang diagnostic push")
_Pragma("clang diagnostic ignored \"-Wnon-virtual-dtor\"")
#elif defined(__GNUC__) || defined(__GNUG__)
_Pragma("GCC diagnostic push")
_Pragma("GCC diagnostic ignored \"-Wnon-virtual-dtor\"")
_Pragma("GCC diagnostic ignored \"-Weffc++\"")
#endif

// defining the boost MSM state machine
struct FairMQFSM_ : public msmf::state_machine_def<FairMQFSM_>
{
  public:
    FairMQFSM_()
        : fWorkerThread()
        , fWork()
        , fWorkAvailableCondition()
        , fWorkDoneCondition()
        , fWorkMutex()
        , fWorkerTerminated(false)
        , fWorkActive(false)
        , fWorkAvailable(false)
        , fState()
        , fChangeStateMutex()
        , fStateChangeSignal()
        , fStateChangeSignalsMap()
        {}

    // Destructor
    virtual ~FairMQFSM_() {};

    template <class Event, class FSM>
    void on_entry(Event const&, FSM& fsm)
    {
        LOG(STATE) << "Starting FairMQ state machine";
        fState = IDLE;

        // start a worker thread to execute user states in.
        fsm.fWorkerThread = std::thread(&FairMQFSM_::Worker, &fsm);
    }

    template <class Event, class FSM>
    void on_exit(Event const&, FSM& /*fsm*/)
    {
        LOG(STATE) << "Exiting FairMQ state machine";
    }

    // The list of FSM states
    struct OK_FSM : public msmf::state<> {};
    struct ERROR_FSM : public msmf::terminate_state<> {};

    struct IDLE_FSM : public msmf::state<> {};
    struct INITIALIZING_DEVICE_FSM : public msmf::state<> {};
    struct DEVICE_READY_FSM : public msmf::state<> {};
    struct INITIALIZING_TASK_FSM : public msmf::state<> {};
    struct READY_FSM : public msmf::state<> {};
    struct RUNNING_FSM : public msmf::state<> {};
    struct PAUSED_FSM : public msmf::state<> {};
    struct RESETTING_TASK_FSM : public msmf::state<> {};
    struct RESETTING_DEVICE_FSM : public msmf::state<> {};
    struct EXITING_FSM : public msmf::state<> {};

    // Define initial states
    typedef mpl::vector<IDLE_FSM, OK_FSM> initial_state;

    // Actions
    struct IdleFct
    {
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(EVT const&, FSM& fsm, SourceState&, TargetState&)
        {
            LOG(STATE) << "Entering IDLE state";
            fsm.fState = IDLE;
            fsm.CallStateChangeCallbacks(IDLE);
        }
    };

    struct InitDeviceFct
    {
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(EVT const&, FSM& fsm, SourceState&, TargetState&)
        {
            LOG(STATE) << "Entering INITIALIZING DEVICE state";
            fsm.fState = INITIALIZING_DEVICE;

            std::unique_lock<std::mutex> lock(fsm.fWorkMutex);
            while (fsm.fWorkActive)
            {
                fsm.fWorkDoneCondition.wait(lock);
            }
            fsm.fWorkAvailable = true;
            fsm.fWork = std::bind(&FairMQFSM_::InitWrapper, &fsm);
            fsm.fWorkAvailableCondition.notify_one();
        }
    };

    struct DeviceReadyFct
    {
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(EVT const&, FSM& fsm, SourceState&, TargetState&)
        {
            LOG(STATE) << "Entering DEVICE READY state";
            fsm.fState = DEVICE_READY;
            fsm.CallStateChangeCallbacks(DEVICE_READY);
        }
    };

    struct InitTaskFct
    {
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(EVT const&, FSM& fsm, SourceState&, TargetState&)
        {
            LOG(STATE) << "Entering INITIALIZING TASK state";
            fsm.fState = INITIALIZING_TASK;

            std::unique_lock<std::mutex> lock(fsm.fWorkMutex);
            while (fsm.fWorkActive)
            {
                fsm.fWorkDoneCondition.wait(lock);
            }
            fsm.fWorkAvailable = true;
            fsm.fWork = std::bind(&FairMQFSM_::InitTaskWrapper, &fsm);
            fsm.fWorkAvailableCondition.notify_one();
        }
    };

    struct ReadyFct
    {
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(EVT const&, FSM& fsm, SourceState&, TargetState&)
        {
            LOG(STATE) << "Entering READY state";
            fsm.fState = READY;
            fsm.CallStateChangeCallbacks(READY);
        }
    };

    struct RunFct
    {
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(EVT const&, FSM& fsm, SourceState&, TargetState&)
        {
            LOG(STATE) << "Entering RUNNING state";
            fsm.fState = RUNNING;

            std::unique_lock<std::mutex> lock(fsm.fWorkMutex);
            while (fsm.fWorkActive)
            {
                fsm.fWorkDoneCondition.wait(lock);
            }
            fsm.fWorkAvailable = true;
            fsm.fWork = std::bind(&FairMQFSM_::RunWrapper, &fsm);
            fsm.fWorkAvailableCondition.notify_one();
        }
    };

    struct PauseFct
    {
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(EVT const&, FSM& fsm, SourceState&, TargetState&)
        {
            LOG(STATE) << "Entering PAUSED state";
            fsm.fState = PAUSED;

            fsm.Unblock();
            std::unique_lock<std::mutex> lock(fsm.fWorkMutex);
            while (fsm.fWorkActive)
            {
                fsm.fWorkDoneCondition.wait(lock);
            }
            fsm.fWorkAvailable = true;
            fsm.fWork = std::bind(&FairMQFSM_::PauseWrapper, &fsm);
            fsm.fWorkAvailableCondition.notify_one();
        }
    };

    struct ResumeFct
    {
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(EVT const&, FSM& fsm, SourceState&, TargetState&)
        {
            LOG(STATE) << "Entering RUNNING state";
            fsm.fState = RUNNING;

            std::unique_lock<std::mutex> lock(fsm.fWorkMutex);
            while (fsm.fWorkActive)
            {
                fsm.fWorkDoneCondition.wait(lock);
            }
            fsm.fWorkAvailable = true;
            fsm.fWork = std::bind(&FairMQFSM_::RunWrapper, &fsm);
            fsm.fWorkAvailableCondition.notify_one();
        }
    };

    struct StopFct
    {
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(EVT const&, FSM& fsm, SourceState&, TargetState&)
        {
            LOG(STATE) << "Entering READY state";
            fsm.fState = READY;
            fsm.CallStateChangeCallbacks(READY);

            fsm.Unblock();
            std::unique_lock<std::mutex> lock(fsm.fWorkMutex);
            while (fsm.fWorkActive)
            {
                fsm.fWorkDoneCondition.wait(lock);
            }
        }
    };

    struct InternalStopFct
    {
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(EVT const&, FSM& fsm, SourceState&, TargetState&)
        {
            LOG(STATE) << "RUNNING state finished without an external event, entering READY state";
            fsm.fState = READY;
            fsm.CallStateChangeCallbacks(READY);

            fsm.Unblock();
        }
    };

    struct ResetTaskFct
    {
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(EVT const&, FSM& fsm, SourceState&, TargetState&)
        {
            LOG(STATE) << "Entering RESETTING TASK state";
            fsm.fState = RESETTING_TASK;

            std::unique_lock<std::mutex> lock(fsm.fWorkMutex);
            while (fsm.fWorkActive)
            {
                fsm.fWorkDoneCondition.wait(lock);
            }
            fsm.fWorkAvailable = true;
            fsm.fWork = std::bind(&FairMQFSM_::ResetTaskWrapper, &fsm);
            fsm.fWorkAvailableCondition.notify_one();
        }
    };

    struct ResetDeviceFct
    {
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(EVT const&, FSM& fsm, SourceState&, TargetState&)
        {
            LOG(STATE) << "Entering RESETTING DEVICE state";
            fsm.fState = RESETTING_DEVICE;

            std::unique_lock<std::mutex> lock(fsm.fWorkMutex);
            while (fsm.fWorkActive)
            {
                fsm.fWorkDoneCondition.wait(lock);
            }
            fsm.fWorkAvailable = true;
            fsm.fWork = std::bind(&FairMQFSM_::ResetWrapper, &fsm);
            fsm.fWorkAvailableCondition.notify_one();
        }
    };

    struct ExitingFct
    {
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(EVT const&, FSM& fsm, SourceState&, TargetState&)
        {
            LOG(STATE) << "Entering EXITING state";
            fsm.fState = EXITING;
            fsm.CallStateChangeCallbacks(EXITING);

            // terminate worker thread
            {
                std::lock_guard<std::mutex> lock(fsm.fWorkMutex);
                fsm.fWorkerTerminated = true;
                fsm.fWorkAvailableCondition.notify_one();
            }

            // join the worker thread (executing user states)
            if (fsm.fWorkerThread.joinable())
            {
                fsm.fWorkerThread.join();
            }

            fsm.Exit();
        }
    };

    struct ErrorFoundFct
    {
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(EVT const&, FSM& fsm, SourceState&, TargetState&)
        {
            LOG(STATE) << "Entering ERROR state";
            fsm.fState = ERROR;
            fsm.CallStateChangeCallbacks(ERROR);
        }
    };

    // actions to be overwritten by derived classes
    virtual void InitWrapper() {}
    virtual void Init() {}
    virtual void InitTaskWrapper() {}
    virtual void InitTask() {}
    virtual void RunWrapper() {}
    virtual void Run() {}
    virtual void PauseWrapper() {}
    virtual void Pause() {}
    virtual void ResetWrapper() {}
    virtual void Reset() {}
    virtual void ResetTaskWrapper() {}
    virtual void ResetTask() {}
    virtual void Exit() {}
    virtual void Unblock() {} // Method to send commands.

    void Worker()
    {
        while (true)
        {
            {
                std::unique_lock<std::mutex> lock(fWorkMutex);
                // Wait for work to be done.
                while (!fWorkAvailable && !fWorkerTerminated)
                {
                    fWorkAvailableCondition.wait(lock);
                }

                if (fWorkerTerminated)
                {
                    break;
                }

                fWorkActive = true;
            }

            fWork();

            std::lock_guard<std::mutex> lock(fWorkMutex);
            fWorkActive = false;
            fWorkAvailable = false;
            fWorkDoneCondition.notify_one();
        }
    }

    // Transition table for FairMQFSM
    struct transition_table : mpl::vector<
        //        Start                    Event                  Next                     Action           Guard
        //        +------------------------+----------------------+------------------------+----------------+---------+
        msmf::Row<IDLE_FSM,                INIT_DEVICE,           INITIALIZING_DEVICE_FSM, InitDeviceFct,   msmf::none>,
        msmf::Row<IDLE_FSM,                END,                   EXITING_FSM,             ExitingFct,      msmf::none>,
        msmf::Row<INITIALIZING_DEVICE_FSM, internal_DEVICE_READY, DEVICE_READY_FSM,        DeviceReadyFct,  msmf::none>,
        msmf::Row<DEVICE_READY_FSM,        INIT_TASK,             INITIALIZING_TASK_FSM,   InitTaskFct,     msmf::none>,
        msmf::Row<DEVICE_READY_FSM,        RESET_DEVICE,          RESETTING_DEVICE_FSM,    ResetDeviceFct,  msmf::none>,
        msmf::Row<INITIALIZING_TASK_FSM,   internal_READY,        READY_FSM,               ReadyFct,        msmf::none>,
        msmf::Row<READY_FSM,               RUN,                   RUNNING_FSM,             RunFct,          msmf::none>,
        msmf::Row<READY_FSM,               RESET_TASK,            RESETTING_TASK_FSM,      ResetTaskFct,    msmf::none>,
        msmf::Row<RUNNING_FSM,             PAUSE,                 PAUSED_FSM,              PauseFct,        msmf::none>,
        msmf::Row<RUNNING_FSM,             STOP,                  READY_FSM,               StopFct,         msmf::none>,
        msmf::Row<RUNNING_FSM,             internal_READY,        READY_FSM,               InternalStopFct, msmf::none>,
        msmf::Row<PAUSED_FSM,              RUN,                   RUNNING_FSM,             ResumeFct,       msmf::none>,
        msmf::Row<RESETTING_TASK_FSM,      internal_DEVICE_READY, DEVICE_READY_FSM,        DeviceReadyFct,  msmf::none>,
        msmf::Row<RESETTING_DEVICE_FSM,    internal_IDLE,         IDLE_FSM,                IdleFct,         msmf::none>,
        msmf::Row<OK_FSM,                  ERROR_FOUND,           ERROR_FSM,               ErrorFoundFct,   msmf::none>>
        {};

    // Replaces the default no-transition response.
    template <class FSM, class Event>
    void no_transition(Event const& e, FSM&, int state)
    {
        typedef typename msm::back::recursive_get_transition_table<FSM>::type recursive_stt;
        typedef typename msm::back::generate_state_set<recursive_stt>::type all_states;

        std::string stateName;

        mpl::for_each<all_states, msm::wrap<mpl::placeholders::_1>>(msm::back::get_state_name<recursive_stt>(stateName, state));

        stateName = stateName.substr(24);
        std::size_t pos = stateName.find("_FSME");
        stateName.erase(pos);

        if (stateName == "1RUNNING" || stateName == "6DEVICE_READY" || stateName == "0PAUSED" || stateName == "8RESETTING_TASK" || stateName == "0RESETTING_DEVICE")
        {
            stateName = stateName.substr(1);
        }

        if (stateName != "OK")
        {
            LOG(STATE) << "No transition from state " << stateName << " on event " << e.name();
        }

        // LOG(STATE) << "no transition from state " << GetStateName(state) << " (" << state << ") on event " << e.name();
    }

    // backward compatibility to FairMQStateMachine
    enum State
    {
        OK,
        ERROR,
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

    static std::string GetStateName(const int state)
    {
        switch(state)
        {
            case OK:
                return "OK";
            case ERROR:
                return "ERROR";
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

    std::string GetCurrentStateName() const { return GetStateName(fState); }
    int GetCurrentState() const { return fState; }
    bool CheckCurrentState(int state) const { return state == fState; }
    bool CheckCurrentState(std::string state) const { return state == GetCurrentStateName(); }

    void CallStateChangeCallbacks(const State state) const
    {
        if (!fStateChangeSignal.empty())
        {
            fStateChangeSignal(state);
        }
    }

    // this is to run certain functions in a separate thread
    std::thread fWorkerThread;

    // function to execute user states in a worker thread
    std::function<void(void)> fWork;
    std::condition_variable fWorkAvailableCondition;
    std::condition_variable fWorkDoneCondition;
    std::mutex fWorkMutex;
    bool fWorkerTerminated;
    bool fWorkActive;
    bool fWorkAvailable;

  protected:
    std::atomic<State> fState;
    std::mutex fChangeStateMutex;

    boost::signals2::signal<void(const State)> fStateChangeSignal;
    std::unordered_map<std::string, boost::signals2::connection> fStateChangeSignalsMap;
};

// reactivate the warning for non-virtual destructor
#if defined(__clang__)
_Pragma("clang diagnostic pop")
#elif defined(__GNUC__) || defined(__GNUG__)
_Pragma("GCC diagnostic pop")
#endif

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
        END,
        ERROR_FOUND
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

    void SubscribeToStateChange(const std::string& key, std::function<void(const State)> callback);
    void UnsubscribeFromStateChange(const std::string& key);
};

#endif /* FAIRMQSTATEMACHINE_H_ */
