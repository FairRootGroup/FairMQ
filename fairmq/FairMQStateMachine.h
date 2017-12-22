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

#include <string>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <chrono>
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

namespace msmf = boost::msm::front;

namespace fair
{
namespace mq
{
namespace fsm
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
#endif

// defining the boost MSM state machine
struct FairMQFSM : public msmf::state_machine_def<FairMQFSM>
{
  public:
    FairMQFSM()
        : fState()
        , fChangeStateMutex()
        , fWork()
        , fWorkAvailableCondition()
        , fWorkDoneCondition()
        , fWorkMutex()
        , fWorkerTerminated(false)
        , fWorkActive(false)
        , fWorkAvailable(false)
        , fStateChangeSignal()
        , fStateChangeSignalsMap()
        , fTerminationRequested(false)
        , fWorkerThread()
    {}

    virtual ~FairMQFSM()
    {}

    template<typename Event, typename FSM>
    void on_entry(Event const&, FSM& fsm)
    {
        LOG(state) << "Starting FairMQ state machine";
        fState = IDLE;
        fsm.CallStateChangeCallbacks(IDLE);

        // start a worker thread to execute user states in.
        fsm.fWorkerThread = std::thread(&FairMQFSM::Worker, &fsm);
    }

    template<typename Event, typename FSM>
    void on_exit(Event const&, FSM& /*fsm*/)
    {
        LOG(state) << "Exiting FairMQ state machine";
    }

    // list of FSM states
    struct OK_FSM                  : public msmf::state<> {};
    struct ERROR_FSM               : public msmf::terminate_state<> {};

    struct IDLE_FSM                : public msmf::state<> {};
    struct INITIALIZING_DEVICE_FSM : public msmf::state<> {};
    struct DEVICE_READY_FSM        : public msmf::state<> {};
    struct INITIALIZING_TASK_FSM   : public msmf::state<> {};
    struct READY_FSM               : public msmf::state<> {};
    struct RUNNING_FSM             : public msmf::state<> {};
    struct PAUSED_FSM              : public msmf::state<> {};
    struct RESETTING_TASK_FSM      : public msmf::state<> {};
    struct RESETTING_DEVICE_FSM    : public msmf::state<> {};
    struct EXITING_FSM             : public msmf::state<> {};

    // initial states
    using initial_state = boost::mpl::vector<IDLE_FSM, OK_FSM>;

    // actions
    struct IdleFct
    {
        template<typename EVT, typename FSM, typename SourceState, typename TargetState>
        void operator()(EVT const&, FSM& fsm, SourceState&, TargetState&)
        {
            LOG(state) << "Entering IDLE state";
            fsm.fState = IDLE;
        }
    };

    struct InitDeviceFct
    {
        template<typename EVT, typename FSM, typename SourceState, typename TargetState>
        void operator()(EVT const&, FSM& fsm, SourceState&, TargetState&)
        {
            fsm.fState = INITIALIZING_DEVICE;

            std::unique_lock<std::mutex> lock(fsm.fWorkMutex);
            while (fsm.fWorkActive)
            {
                fsm.fWorkDoneCondition.wait(lock);
            }
            fsm.fWorkAvailable = true;
            LOG(state) << "Entering INITIALIZING DEVICE state";
            fsm.fWork = std::bind(&FairMQFSM::InitWrapper, &fsm);
            fsm.fWorkAvailableCondition.notify_one();
        }
    };

    struct DeviceReadyFct
    {
        template<typename EVT, typename FSM, typename SourceState, typename TargetState>
        void operator()(EVT const&, FSM& fsm, SourceState&, TargetState&)
        {
            LOG(state) << "Entering DEVICE READY state";
            fsm.fState = DEVICE_READY;
        }
    };

    struct InitTaskFct
    {
        template<typename EVT, typename FSM, typename SourceState, typename TargetState>
        void operator()(EVT const&, FSM& fsm, SourceState&, TargetState&)
        {
            fsm.fState = INITIALIZING_TASK;

            std::unique_lock<std::mutex> lock(fsm.fWorkMutex);
            while (fsm.fWorkActive)
            {
                fsm.fWorkDoneCondition.wait(lock);
            }
            fsm.fWorkAvailable = true;
            LOG(state) << "Entering INITIALIZING TASK state";
            fsm.fWork = std::bind(&FairMQFSM::InitTaskWrapper, &fsm);
            fsm.fWorkAvailableCondition.notify_one();
        }
    };

    struct ReadyFct
    {
        template<typename EVT, typename FSM, typename SourceState, typename TargetState>
        void operator()(EVT const&, FSM& fsm, SourceState&, TargetState&)
        {
            LOG(state) << "Entering READY state";
            fsm.fState = READY;
        }
    };

    struct RunFct
    {
        template<typename EVT, typename FSM, typename SourceState, typename TargetState>
        void operator()(EVT const&, FSM& fsm, SourceState&, TargetState&)
        {
            fsm.fState = RUNNING;

            std::unique_lock<std::mutex> lock(fsm.fWorkMutex);
            while (fsm.fWorkActive)
            {
                fsm.fWorkDoneCondition.wait(lock);
            }
            fsm.fWorkAvailable = true;
            LOG(state) << "Entering RUNNING state";
            fsm.fWork = std::bind(&FairMQFSM::RunWrapper, &fsm);
            fsm.fWorkAvailableCondition.notify_one();
        }
    };

    struct PauseFct
    {
        template<typename EVT, typename FSM, typename SourceState, typename TargetState>
        void operator()(EVT const&, FSM& fsm, SourceState&, TargetState&)
        {
            fsm.fState = PAUSED;

            fsm.Unblock();
            std::unique_lock<std::mutex> lock(fsm.fWorkMutex);
            while (fsm.fWorkActive)
            {
                fsm.fWorkDoneCondition.wait(lock);
            }
            fsm.fWorkAvailable = true;
            LOG(state) << "Entering PAUSED state";
            fsm.fWork = std::bind(&FairMQFSM::PauseWrapper, &fsm);
            fsm.fWorkAvailableCondition.notify_one();
        }
    };

    struct ResumeFct
    {
        template<typename EVT, typename FSM, typename SourceState, typename TargetState>
        void operator()(EVT const&, FSM& fsm, SourceState&, TargetState&)
        {
            fsm.fState = RUNNING;

            std::unique_lock<std::mutex> lock(fsm.fWorkMutex);
            while (fsm.fWorkActive)
            {
                fsm.fWorkDoneCondition.wait(lock);
            }
            fsm.fWorkAvailable = true;
            LOG(state) << "Entering RUNNING state";
            fsm.fWork = std::bind(&FairMQFSM::RunWrapper, &fsm);
            fsm.fWorkAvailableCondition.notify_one();
        }
    };

    struct StopFct
    {
        template<typename EVT, typename FSM, typename SourceState, typename TargetState>
        void operator()(EVT const&, FSM& fsm, SourceState&, TargetState&)
        {
            fsm.fState = READY;

            fsm.Unblock();
            std::unique_lock<std::mutex> lock(fsm.fWorkMutex);
            while (fsm.fWorkActive)
            {
                fsm.fWorkDoneCondition.wait(lock);
            }
            LOG(state) << "Entering READY state";
        }
    };

    struct InternalStopFct
    {
        template<typename EVT, typename FSM, typename SourceState, typename TargetState>
        void operator()(EVT const&, FSM& fsm, SourceState&, TargetState&)
        {
            fsm.fState = READY;
            fsm.Unblock();
            LOG(state) << "RUNNING state finished without an external event, entering READY state";
        }
    };

    struct ResetTaskFct
    {
        template<typename EVT, typename FSM, typename SourceState, typename TargetState>
        void operator()(EVT const&, FSM& fsm, SourceState&, TargetState&)
        {
            fsm.fState = RESETTING_TASK;

            std::unique_lock<std::mutex> lock(fsm.fWorkMutex);
            while (fsm.fWorkActive)
            {
                fsm.fWorkDoneCondition.wait(lock);
            }
            fsm.fWorkAvailable = true;
            LOG(state) << "Entering RESETTING TASK state";
            fsm.fWork = std::bind(&FairMQFSM::ResetTaskWrapper, &fsm);
            fsm.fWorkAvailableCondition.notify_one();
        }
    };

    struct ResetDeviceFct
    {
        template<typename EVT, typename FSM, typename SourceState, typename TargetState>
        void operator()(EVT const&, FSM& fsm, SourceState&, TargetState&)
        {
            fsm.fState = RESETTING_DEVICE;

            std::unique_lock<std::mutex> lock(fsm.fWorkMutex);
            while (fsm.fWorkActive)
            {
                fsm.fWorkDoneCondition.wait(lock);
            }
            fsm.fWorkAvailable = true;
            LOG(state) << "Entering RESETTING DEVICE state";
            fsm.fWork = std::bind(&FairMQFSM::ResetWrapper, &fsm);
            fsm.fWorkAvailableCondition.notify_one();
        }
    };

    struct ExitingFct
    {
        template<typename EVT, typename FSM, typename SourceState, typename TargetState>
        void operator()(EVT const&, FSM& fsm, SourceState&, TargetState&)
        {
            LOG(state) << "Entering EXITING state";
            fsm.fState = EXITING;
            fsm.fTerminationRequested = true;
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
        template<typename EVT, typename FSM, typename SourceState, typename TargetState>
        void operator()(EVT const&, FSM& fsm, SourceState&, TargetState&)
        {
            LOG(state) << "Entering ERROR state";
            fsm.fState = Error;
            fsm.CallStateChangeCallbacks(Error);
        }
    };

    // Transition table for FairMQFSM
    struct transition_table : boost::mpl::vector<
        //        Start                    Event                  Next                     Action           Guard
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

    // replaces the default no-transition response.
    template<typename FSM, typename Event>
    void no_transition(Event const& e, FSM&, int state)
    {
        using recursive_stt = typename boost::msm::back::recursive_get_transition_table<FSM>::type;
        using all_states = typename boost::msm::back::generate_state_set<recursive_stt>::type;

        std::string stateName;

        boost::mpl::for_each<all_states, boost::msm::wrap<boost::mpl::placeholders::_1>>(boost::msm::back::get_state_name<recursive_stt>(stateName, state));

        stateName = stateName.substr(24);
        std::size_t pos = stateName.find("_FSME");
        stateName.erase(pos);

        if (stateName == "1RUNNING" || stateName == "6DEVICE_READY" || stateName == "0PAUSED" || stateName == "8RESETTING_TASK" || stateName == "0RESETTING_DEVICE")
        {
            stateName = stateName.substr(1);
        }

        if (stateName != "OK")
        {
            LOG(state) << "No transition from state " << stateName << " on event " << e.name();
        }

        // LOG(state) << "no transition from state " << GetStateName(state) << " (" << state << ") on event " << e.name();
    }

    // backward compatibility to FairMQStateMachine
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

    static std::string GetStateName(const int state)
    {
        switch(state)
        {
            case OK:
                return "OK";
            case Error:
                return "Error";
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
        return state == fState;
    }
    bool CheckCurrentState(std::string state) const
    {
        return state == GetCurrentStateName();
    }

    // actions to be overwritten by derived classes
    virtual void InitWrapper() {}
    virtual void InitTaskWrapper() {}
    virtual void RunWrapper() {}
    virtual void PauseWrapper() {}
    virtual void ResetWrapper() {}
    virtual void ResetTaskWrapper() {}
    virtual void Exit() {}
    virtual void Unblock() {}

    bool Terminated()
    {
        return fTerminationRequested;
    }

  protected:
    std::atomic<State> fState;
    std::mutex fChangeStateMutex;

    // function to execute user states in a worker thread
    std::function<void(void)> fWork;
    std::condition_variable fWorkAvailableCondition;
    std::condition_variable fWorkDoneCondition;
    std::mutex fWorkMutex;
    bool fWorkerTerminated;
    bool fWorkActive;
    bool fWorkAvailable;

    boost::signals2::signal<void(const State)> fStateChangeSignal;
    std::unordered_map<std::string, boost::signals2::connection> fStateChangeSignalsMap;
    std::atomic<bool> fTerminationRequested;

    void CallStateChangeCallbacks(const State state) const
    {
        if (!fStateChangeSignal.empty())
        {
            fStateChangeSignal(state);
        }
    }

  private:
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

            {
                std::lock_guard<std::mutex> lock(fWorkMutex);
                fWorkActive = false;
                fWorkAvailable = false;
                fWorkDoneCondition.notify_one();
            }
            CallStateChangeCallbacks(fState);
        }
    }

    // run state handlers in a separate thread
    std::thread fWorkerThread;
};

// reactivate the warning for non-virtual destructor
#if defined(__clang__)
_Pragma("clang diagnostic pop")
#elif defined(__GNUC__) || defined(__GNUG__)
_Pragma("GCC diagnostic pop")
#endif

} // namespace fsm
} // namespace mq
} // namespace fair

class FairMQStateMachine : public boost::msm::back::state_machine<fair::mq::fsm::FairMQFSM>
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

    int GetInterfaceVersion() const;

    bool ChangeState(int event);
    bool ChangeState(const std::string& event);

    void WaitForEndOfState(int event);
    void WaitForEndOfState(const std::string& event);

    bool WaitForEndOfStateForMs(int event, int durationInMs);
    bool WaitForEndOfStateForMs(const std::string& event, int durationInMs);

    void SubscribeToStateChange(const std::string& key, std::function<void(const State)> callback);
    void UnsubscribeFromStateChange(const std::string& key);

  private:
    int GetEventNumber(const std::string& event);
};

#endif /* FAIRMQSTATEMACHINE_H_ */
