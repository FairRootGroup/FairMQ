/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQStateMachine.cxx
 *
 * @since 2012-10-25
 * @author D. Klein, A. Rybalchenko
 */

#include "FairMQStateMachine.h"

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

#include <atomic>
#include <condition_variable>
#include <thread>
#include <chrono>
#include <unordered_map>

using namespace std;

namespace msmf = boost::msm::front;

namespace fair
{
namespace mq
{
namespace fsm
{

// defining events for the boost MSM state machine
struct INIT_DEVICE_E           { string name() const { return "INIT_DEVICE"; } };
struct internal_DEVICE_READY_E { string name() const { return "internal_DEVICE_READY"; } };
struct INIT_TASK_E             { string name() const { return "INIT_TASK"; } };
struct internal_READY_E        { string name() const { return "internal_READY"; } };
struct RUN_E                   { string name() const { return "RUN"; } };
struct PAUSE_E                 { string name() const { return "PAUSE"; } };
struct STOP_E                  { string name() const { return "STOP"; } };
struct RESET_TASK_E            { string name() const { return "RESET_TASK"; } };
struct RESET_DEVICE_E          { string name() const { return "RESET_DEVICE"; } };
struct internal_IDLE_E         { string name() const { return "internal_IDLE"; } };
struct END_E                   { string name() const { return "END"; } };
struct ERROR_FOUND_E           { string name() const { return "ERROR_FOUND"; } };

// deactivate the warning for non-virtual destructor thrown in the boost library
#if defined(__clang__)
_Pragma("clang diagnostic push")
_Pragma("clang diagnostic ignored \"-Wnon-virtual-dtor\"")
#elif defined(__GNUC__) || defined(__GNUG__)
_Pragma("GCC diagnostic push")
_Pragma("GCC diagnostic ignored \"-Wnon-virtual-dtor\"")
#endif

// defining the boost MSM state machine
struct Machine_ : public msmf::state_machine_def<Machine_>
{
  public:
    Machine_()
        : fWork()
        , fWorkAvailableCondition()
        , fWorkDoneCondition()
        , fWorkMutex()
        , fWorkerTerminated(false)
        , fWorkActive(false)
        , fWorkAvailable(false)
        , fStateChangeSignal()
        , fStateChangeSignalsMap()
        , fTerminationRequested(false)
        , fState()
        , fWorkerThread()
    {}

    virtual ~Machine_()
    {}

    template<typename Event, typename FSM>
    void on_entry(Event const&, FSM& fsm)
    {
        LOG(state) << "Starting FairMQ state machine";
        fState = FairMQStateMachine::IDLE;
        fsm.CallStateChangeCallbacks(FairMQStateMachine::IDLE);

        // start a worker thread to execute user states in.
        fsm.fWorkerThread = thread(&Machine_::Worker, &fsm);
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
            fsm.fState = FairMQStateMachine::IDLE;
        }
    };

    struct InitDeviceFct
    {
        template<typename EVT, typename FSM, typename SourceState, typename TargetState>
        void operator()(EVT const&, FSM& fsm, SourceState&, TargetState&)
        {
            fsm.fState = FairMQStateMachine::INITIALIZING_DEVICE;

            unique_lock<mutex> lock(fsm.fWorkMutex);
            while (fsm.fWorkActive)
            {
                fsm.fWorkDoneCondition.wait(lock);
            }
            fsm.fWorkAvailable = true;
            LOG(state) << "Entering INITIALIZING DEVICE state";
            fsm.fWork = fsm.fInitWrapperHandler;
            fsm.fWorkAvailableCondition.notify_one();
        }
    };

    struct DeviceReadyFct
    {
        template<typename EVT, typename FSM, typename SourceState, typename TargetState>
        void operator()(EVT const&, FSM& fsm, SourceState&, TargetState&)
        {
            LOG(state) << "Entering DEVICE READY state";
            fsm.fState = FairMQStateMachine::DEVICE_READY;
        }
    };

    struct InitTaskFct
    {
        template<typename EVT, typename FSM, typename SourceState, typename TargetState>
        void operator()(EVT const&, FSM& fsm, SourceState&, TargetState&)
        {
            fsm.fState = FairMQStateMachine::INITIALIZING_TASK;

            unique_lock<mutex> lock(fsm.fWorkMutex);
            while (fsm.fWorkActive)
            {
                fsm.fWorkDoneCondition.wait(lock);
            }
            fsm.fWorkAvailable = true;
            LOG(state) << "Entering INITIALIZING TASK state";
            fsm.fWork = fsm.fInitTaskWrapperHandler;
            fsm.fWorkAvailableCondition.notify_one();
        }
    };

    struct ReadyFct
    {
        template<typename EVT, typename FSM, typename SourceState, typename TargetState>
        void operator()(EVT const&, FSM& fsm, SourceState&, TargetState&)
        {
            LOG(state) << "Entering READY state";
            fsm.fState = FairMQStateMachine::READY;
        }
    };

    struct RunFct
    {
        template<typename EVT, typename FSM, typename SourceState, typename TargetState>
        void operator()(EVT const&, FSM& fsm, SourceState&, TargetState&)
        {
            fsm.fState = FairMQStateMachine::RUNNING;

            unique_lock<mutex> lock(fsm.fWorkMutex);
            while (fsm.fWorkActive)
            {
                fsm.fWorkDoneCondition.wait(lock);
            }
            fsm.fWorkAvailable = true;
            LOG(state) << "Entering RUNNING state";
            fsm.fWork = fsm.fRunWrapperHandler;
            fsm.fWorkAvailableCondition.notify_one();
        }
    };

    struct PauseFct
    {
        template<typename EVT, typename FSM, typename SourceState, typename TargetState>
        void operator()(EVT const&, FSM& fsm, SourceState&, TargetState&)
        {
            fsm.fState = FairMQStateMachine::PAUSED;

            fsm.fUnblockHandler();
            unique_lock<mutex> lock(fsm.fWorkMutex);
            while (fsm.fWorkActive)
            {
                fsm.fWorkDoneCondition.wait(lock);
            }
            fsm.fWorkAvailable = true;
            LOG(state) << "Entering PAUSED state";
            fsm.fWork = fsm.fPauseWrapperHandler;
            fsm.fWorkAvailableCondition.notify_one();
        }
    };

    struct ResumeFct
    {
        template<typename EVT, typename FSM, typename SourceState, typename TargetState>
        void operator()(EVT const&, FSM& fsm, SourceState&, TargetState&)
        {
            fsm.fState = FairMQStateMachine::RUNNING;

            unique_lock<mutex> lock(fsm.fWorkMutex);
            while (fsm.fWorkActive)
            {
                fsm.fWorkDoneCondition.wait(lock);
            }
            fsm.fWorkAvailable = true;
            LOG(state) << "Entering RUNNING state";
            fsm.fWork = fsm.fRunWrapperHandler;
            fsm.fWorkAvailableCondition.notify_one();
        }
    };

    struct StopFct
    {
        template<typename EVT, typename FSM, typename SourceState, typename TargetState>
        void operator()(EVT const&, FSM& fsm, SourceState&, TargetState&)
        {
            fsm.fState = FairMQStateMachine::READY;

            fsm.fUnblockHandler();
            unique_lock<mutex> lock(fsm.fWorkMutex);
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
            fsm.fState = FairMQStateMachine::READY;
            fsm.fUnblockHandler();
            LOG(state) << "RUNNING state finished without an external event, entering READY state";
        }
    };

    struct ResetTaskFct
    {
        template<typename EVT, typename FSM, typename SourceState, typename TargetState>
        void operator()(EVT const&, FSM& fsm, SourceState&, TargetState&)
        {
            fsm.fState = FairMQStateMachine::RESETTING_TASK;

            unique_lock<mutex> lock(fsm.fWorkMutex);
            while (fsm.fWorkActive)
            {
                fsm.fWorkDoneCondition.wait(lock);
            }
            fsm.fWorkAvailable = true;
            LOG(state) << "Entering RESETTING TASK state";
            fsm.fWork = fsm.fResetTaskWrapperHandler;
            fsm.fWorkAvailableCondition.notify_one();
        }
    };

    struct ResetDeviceFct
    {
        template<typename EVT, typename FSM, typename SourceState, typename TargetState>
        void operator()(EVT const&, FSM& fsm, SourceState&, TargetState&)
        {
            fsm.fState = FairMQStateMachine::RESETTING_DEVICE;

            unique_lock<mutex> lock(fsm.fWorkMutex);
            while (fsm.fWorkActive)
            {
                fsm.fWorkDoneCondition.wait(lock);
            }
            fsm.fWorkAvailable = true;
            LOG(state) << "Entering RESETTING DEVICE state";
            fsm.fWork = fsm.fResetWrapperHandler;
            fsm.fWorkAvailableCondition.notify_one();
        }
    };

    struct ExitingFct
    {
        template<typename EVT, typename FSM, typename SourceState, typename TargetState>
        void operator()(EVT const&, FSM& fsm, SourceState&, TargetState&)
        {
            LOG(state) << "Entering EXITING state";
            fsm.fState = FairMQStateMachine::EXITING;
            fsm.fTerminationRequested = true;
            fsm.CallStateChangeCallbacks(FairMQStateMachine::EXITING);

            // terminate worker thread
            {
                lock_guard<mutex> lock(fsm.fWorkMutex);
                fsm.fWorkerTerminated = true;
                fsm.fWorkAvailableCondition.notify_one();
            }

            // join the worker thread (executing user states)
            if (fsm.fWorkerThread.joinable())
            {
                fsm.fWorkerThread.join();
            }

            fsm.fExitHandler();
        }
    };

    struct ErrorFoundFct
    {
        template<typename EVT, typename FSM, typename SourceState, typename TargetState>
        void operator()(EVT const&, FSM& fsm, SourceState&, TargetState&)
        {
            LOG(state) << "Entering ERROR state";
            fsm.fState = FairMQStateMachine::Error;
            fsm.CallStateChangeCallbacks(FairMQStateMachine::Error);
        }
    };

    // Transition table for Machine_
    struct transition_table : boost::mpl::vector<
        //        Start                    Event                    Next                     Action           Guard
        msmf::Row<IDLE_FSM,                INIT_DEVICE_E,           INITIALIZING_DEVICE_FSM, InitDeviceFct,   msmf::none>,
        msmf::Row<IDLE_FSM,                END_E,                   EXITING_FSM,             ExitingFct,      msmf::none>,
        msmf::Row<INITIALIZING_DEVICE_FSM, internal_DEVICE_READY_E, DEVICE_READY_FSM,        DeviceReadyFct,  msmf::none>,
        msmf::Row<DEVICE_READY_FSM,        INIT_TASK_E,             INITIALIZING_TASK_FSM,   InitTaskFct,     msmf::none>,
        msmf::Row<DEVICE_READY_FSM,        RESET_DEVICE_E,          RESETTING_DEVICE_FSM,    ResetDeviceFct,  msmf::none>,
        msmf::Row<INITIALIZING_TASK_FSM,   internal_READY_E,        READY_FSM,               ReadyFct,        msmf::none>,
        msmf::Row<READY_FSM,               RUN_E,                   RUNNING_FSM,             RunFct,          msmf::none>,
        msmf::Row<READY_FSM,               RESET_TASK_E,            RESETTING_TASK_FSM,      ResetTaskFct,    msmf::none>,
        msmf::Row<RUNNING_FSM,             PAUSE_E,                 PAUSED_FSM,              PauseFct,        msmf::none>,
        msmf::Row<RUNNING_FSM,             STOP_E,                  READY_FSM,               StopFct,         msmf::none>,
        msmf::Row<RUNNING_FSM,             internal_READY_E,        READY_FSM,               InternalStopFct, msmf::none>,
        msmf::Row<PAUSED_FSM,              RUN_E,                   RUNNING_FSM,             ResumeFct,       msmf::none>,
        msmf::Row<RESETTING_TASK_FSM,      internal_DEVICE_READY_E, DEVICE_READY_FSM,        DeviceReadyFct,  msmf::none>,
        msmf::Row<RESETTING_DEVICE_FSM,    internal_IDLE_E,         IDLE_FSM,                IdleFct,         msmf::none>,
        msmf::Row<OK_FSM,                  ERROR_FOUND_E,           ERROR_FSM,               ErrorFoundFct,   msmf::none>>
    {};

    // replaces the default no-transition response.
    template<typename FSM, typename Event>
    void no_transition(Event const& e, FSM&, int state)
    {
        using recursive_stt = typename boost::msm::back::recursive_get_transition_table<FSM>::type;
        using all_states = typename boost::msm::back::generate_state_set<recursive_stt>::type;

        string stateName;

        boost::mpl::for_each<all_states, boost::msm::wrap<boost::mpl::placeholders::_1>>(boost::msm::back::get_state_name<recursive_stt>(stateName, state));

        stateName = stateName.substr(24);
        size_t pos = stateName.find("_FSME");
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

    static string GetStateName(const int state)
    {
        switch(state)
        {
            case FairMQStateMachine::OK:
                return "OK";
            case FairMQStateMachine::Error:
                return "Error";
            case FairMQStateMachine::IDLE:
                return "IDLE";
            case FairMQStateMachine::INITIALIZING_DEVICE:
                return "INITIALIZING_DEVICE";
            case FairMQStateMachine::DEVICE_READY:
                return "DEVICE_READY";
            case FairMQStateMachine::INITIALIZING_TASK:
                return "INITIALIZING_TASK";
            case FairMQStateMachine::READY:
                return "READY";
            case FairMQStateMachine::RUNNING:
                return "RUNNING";
            case FairMQStateMachine::PAUSED:
                return "PAUSED";
            case FairMQStateMachine::RESETTING_TASK:
                return "RESETTING_TASK";
            case FairMQStateMachine::RESETTING_DEVICE:
                return "RESETTING_DEVICE";
            case FairMQStateMachine::EXITING:
                return "EXITING";
            default:
                return "requested name for non-existent state...";
        }
    }

    void CallStateChangeCallbacks(const FairMQStateMachine::State state) const
    {
        if (!fStateChangeSignal.empty())
        {
            fStateChangeSignal(state);
        }
    }

    function<void(void)> fInitWrapperHandler;
    function<void(void)> fInitTaskWrapperHandler;
    function<void(void)> fRunWrapperHandler;
    function<void(void)> fPauseWrapperHandler;
    function<void(void)> fResetWrapperHandler;
    function<void(void)> fResetTaskWrapperHandler;
    function<void(void)> fExitHandler;
    function<void(void)> fUnblockHandler;

    // function to execute user states in a worker thread
    function<void(void)> fWork;
    condition_variable fWorkAvailableCondition;
    condition_variable fWorkDoneCondition;
    mutex fWorkMutex;
    bool fWorkerTerminated;
    bool fWorkActive;
    bool fWorkAvailable;

    boost::signals2::signal<void(const FairMQStateMachine::State)> fStateChangeSignal;
    unordered_map<string, boost::signals2::connection> fStateChangeSignalsMap;
    atomic<bool> fTerminationRequested;

    atomic<FairMQStateMachine::State> fState;

  private:
    void Worker()
    {
        while (true)
        {
            {
                unique_lock<mutex> lock(fWorkMutex);
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
                lock_guard<mutex> lock(fWorkMutex);
                fWorkActive = false;
                fWorkAvailable = false;
                fWorkDoneCondition.notify_one();
            }
            CallStateChangeCallbacks(fState);
        }
    }

    // run state handlers in a separate thread
    thread fWorkerThread;
}; // Machine_

using FairMQFSM = boost::msm::back::state_machine<Machine_>;

// reactivate the warning for non-virtual destructor
#if defined(__clang__)
_Pragma("clang diagnostic pop")
#elif defined(__GNUC__) || defined(__GNUG__)
_Pragma("GCC diagnostic pop")
#endif

} // namespace fsm
} // namespace mq
} // namespace fair

using namespace fair::mq::fsm;

FairMQStateMachine::FairMQStateMachine()
    : fChangeStateMutex()
    , fFsm(new FairMQFSM)
{
    static_pointer_cast<FairMQFSM>(fFsm)->fInitWrapperHandler = bind(&FairMQStateMachine::InitWrapper, this);
    static_pointer_cast<FairMQFSM>(fFsm)->fInitTaskWrapperHandler = bind(&FairMQStateMachine::InitTaskWrapper, this);
    static_pointer_cast<FairMQFSM>(fFsm)->fRunWrapperHandler = bind(&FairMQStateMachine::RunWrapper, this);
    static_pointer_cast<FairMQFSM>(fFsm)->fPauseWrapperHandler = bind(&FairMQStateMachine::PauseWrapper, this);
    static_pointer_cast<FairMQFSM>(fFsm)->fResetWrapperHandler = bind(&FairMQStateMachine::ResetWrapper, this);
    static_pointer_cast<FairMQFSM>(fFsm)->fResetTaskWrapperHandler = bind(&FairMQStateMachine::ResetTaskWrapper, this);
    static_pointer_cast<FairMQFSM>(fFsm)->fExitHandler = bind(&FairMQStateMachine::Exit, this);
    static_pointer_cast<FairMQFSM>(fFsm)->fUnblockHandler = bind(&FairMQStateMachine::Unblock, this);

    static_pointer_cast<FairMQFSM>(fFsm)->start();
}

FairMQStateMachine::~FairMQStateMachine()
{
    static_pointer_cast<FairMQFSM>(fFsm)->stop();
}

int FairMQStateMachine::GetInterfaceVersion() const
{
    return FAIRMQ_INTERFACE_VERSION;
}

bool FairMQStateMachine::ChangeState(int event)
{
    try
    {
        switch (event)
        {
            case INIT_DEVICE:
            {
                lock_guard<mutex> lock(fChangeStateMutex);
                static_pointer_cast<FairMQFSM>(fFsm)->process_event(INIT_DEVICE_E());
                return true;
            }
            case internal_DEVICE_READY:
            {
                lock_guard<mutex> lock(fChangeStateMutex);
                static_pointer_cast<FairMQFSM>(fFsm)->process_event(internal_DEVICE_READY_E());
                return true;
            }
            case INIT_TASK:
            {
                lock_guard<mutex> lock(fChangeStateMutex);
                static_pointer_cast<FairMQFSM>(fFsm)->process_event(INIT_TASK_E());
                return true;
            }
            case internal_READY:
            {
                lock_guard<mutex> lock(fChangeStateMutex);
                static_pointer_cast<FairMQFSM>(fFsm)->process_event(internal_READY_E());
                return true;
            }
            case RUN:
            {
                lock_guard<mutex> lock(fChangeStateMutex);
                static_pointer_cast<FairMQFSM>(fFsm)->process_event(RUN_E());
                return true;
            }
            case PAUSE:
            {
                lock_guard<mutex> lock(fChangeStateMutex);
                static_pointer_cast<FairMQFSM>(fFsm)->process_event(PAUSE_E());
                return true;
            }
            case STOP:
            {
                lock_guard<mutex> lock(fChangeStateMutex);
                static_pointer_cast<FairMQFSM>(fFsm)->process_event(STOP_E());
                return true;
            }
            case RESET_DEVICE:
            {
                lock_guard<mutex> lock(fChangeStateMutex);
                static_pointer_cast<FairMQFSM>(fFsm)->process_event(RESET_DEVICE_E());
                return true;
            }
            case RESET_TASK:
            {
                lock_guard<mutex> lock(fChangeStateMutex);
                static_pointer_cast<FairMQFSM>(fFsm)->process_event(RESET_TASK_E());
                return true;
            }
            case internal_IDLE:
            {
                lock_guard<mutex> lock(fChangeStateMutex);
                static_pointer_cast<FairMQFSM>(fFsm)->process_event(internal_IDLE_E());
                return true;
            }
            case END:
            {
                lock_guard<mutex> lock(fChangeStateMutex);
                static_pointer_cast<FairMQFSM>(fFsm)->process_event(END_E());
                return true;
            }
            case ERROR_FOUND:
            {
                lock_guard<mutex> lock(fChangeStateMutex);
                static_pointer_cast<FairMQFSM>(fFsm)->process_event(ERROR_FOUND_E());
                return true;
            }
            default:
            {
                LOG(error) << "Requested state transition with an unsupported event: " << event << endl
                            << "Supported are: INIT_DEVICE, INIT_TASK, RUN, PAUSE, STOP, RESET_TASK, RESET_DEVICE, END, ERROR_FOUND";
                return false;
            }
        }
    }
    catch (exception& e)
    {
        LOG(error) << "Exception in FairMQStateMachine::ChangeState(): " << e.what();
        exit(EXIT_FAILURE);
    }
    return false;
}

bool FairMQStateMachine::ChangeState(const string& event)
{
    return ChangeState(GetEventNumber(event));
}

void FairMQStateMachine::WaitForEndOfState(int event)
{
    try
    {
        switch (event)
        {
            case INIT_DEVICE:
            case INIT_TASK:
            case RUN:
            case RESET_TASK:
            case RESET_DEVICE:
            {
                unique_lock<mutex> lock(static_pointer_cast<FairMQFSM>(fFsm)->fWorkMutex);
                while (static_pointer_cast<FairMQFSM>(fFsm)->fWorkActive || static_pointer_cast<FairMQFSM>(fFsm)->fWorkAvailable)
                {
                    static_pointer_cast<FairMQFSM>(fFsm)->fWorkDoneCondition.wait_for(lock, chrono::seconds(1));
                }

                break;
            }
            default:
                LOG(error) << "Requested state is either synchronous or does not exist.";
                break;
        }
    }
    catch (exception& e)
    {
        LOG(error) << "Exception in FairMQStateMachine::WaitForEndOfState(): " << e.what();
    }
}

void FairMQStateMachine::WaitForEndOfState(const string& event)
{
    return WaitForEndOfState(GetEventNumber(event));
}

bool FairMQStateMachine::WaitForEndOfStateForMs(int event, int durationInMs)
{
    try
    {
        switch (event)
        {
            case INIT_DEVICE:
            case INIT_TASK:
            case RUN:
            case RESET_TASK:
            case RESET_DEVICE:
            {
                unique_lock<mutex> lock(static_pointer_cast<FairMQFSM>(fFsm)->fWorkMutex);
                while (static_pointer_cast<FairMQFSM>(fFsm)->fWorkActive || static_pointer_cast<FairMQFSM>(fFsm)->fWorkAvailable)
                {
                    static_pointer_cast<FairMQFSM>(fFsm)->fWorkDoneCondition.wait_for(lock, chrono::milliseconds(durationInMs));
                    if (static_pointer_cast<FairMQFSM>(fFsm)->fWorkActive)
                    {
                        return false;
                    }
                }
                return true;
            }
            default:
                LOG(error) << "Requested state is either synchronous or does not exist.";
                return false;
        }
    }
    catch (exception& e)
    {
        LOG(error) << "Exception in FairMQStateMachine::WaitForEndOfStateForMs(): " << e.what();
    }
    return false;
}

bool FairMQStateMachine::WaitForEndOfStateForMs(const string& event, int durationInMs)
{
    return WaitForEndOfStateForMs(GetEventNumber(event), durationInMs);
}

void FairMQStateMachine::SubscribeToStateChange(const string& key, function<void(const State)> callback)
{
    static_pointer_cast<FairMQFSM>(fFsm)->fStateChangeSignalsMap.insert({key, static_pointer_cast<FairMQFSM>(fFsm)->fStateChangeSignal.connect(callback)});
}
void FairMQStateMachine::UnsubscribeFromStateChange(const string& key)
{
    if (static_pointer_cast<FairMQFSM>(fFsm)->fStateChangeSignalsMap.count(key))
    {
        static_pointer_cast<FairMQFSM>(fFsm)->fStateChangeSignalsMap.at(key).disconnect();
        static_pointer_cast<FairMQFSM>(fFsm)->fStateChangeSignalsMap.erase(key);
    }
}

void FairMQStateMachine::CallStateChangeCallbacks(const State state) const
{
    static_pointer_cast<FairMQFSM>(fFsm)->CallStateChangeCallbacks(state);
}

string FairMQStateMachine::GetCurrentStateName() const
{
    return static_pointer_cast<FairMQFSM>(fFsm)->GetStateName(static_pointer_cast<FairMQFSM>(fFsm)->fState);
}
int FairMQStateMachine::GetCurrentState() const
{
    return static_pointer_cast<FairMQFSM>(fFsm)->fState;
}
bool FairMQStateMachine::CheckCurrentState(int state) const
{
    return state == static_pointer_cast<FairMQFSM>(fFsm)->fState;
}
bool FairMQStateMachine::CheckCurrentState(string state) const
{
    return state == GetCurrentStateName();
}

bool FairMQStateMachine::Terminated()
{
    return static_pointer_cast<FairMQFSM>(fFsm)->fTerminationRequested;
}

int FairMQStateMachine::GetEventNumber(const string& event)
{
    if (event == "INIT_DEVICE") return INIT_DEVICE;
    if (event == "INIT_TASK") return INIT_TASK;
    if (event == "RUN") return RUN;
    if (event == "PAUSE") return PAUSE;
    if (event == "STOP") return STOP;
    if (event == "RESET_DEVICE") return RESET_DEVICE;
    if (event == "RESET_TASK") return RESET_TASK;
    if (event == "END") return END;
    if (event == "ERROR_FOUND") return ERROR_FOUND;
    LOG(error) << "Requested number for non-existent event... " << event << endl
               << "Supported are: INIT_DEVICE, INIT_TASK, RUN, PAUSE, STOP, RESET_DEVICE, RESET_TASK, END, ERROR_FOUND";
    return -1;
}
