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
#include <fairmq/Tools.h>

// Increase maximum number of boost::msm states (default is 10)
// This #define has to be before any msm header includes
#define FUSION_MAX_VECTOR_SIZE 20

#include <boost/mpl/for_each.hpp>
#include <boost/msm/back/state_machine.hpp>
#include <boost/msm/back/tools.hpp>
#include <boost/msm/back/metafunctions.hpp>
#include <boost/msm/front/state_machine_def.hpp>
#include <boost/msm/front/functor_row.hpp>
#include <boost/core/demangle.hpp>
#include <boost/signals2.hpp> // signal/slot for onStateChange callbacks

#include <atomic>
#include <condition_variable>
#include <chrono>
#include <array>
#include <unordered_map>

using namespace std;
using namespace boost::msm::front;

namespace std
{

template<>
struct hash<FairMQStateMachine::Event> : fair::mq::tools::HashEnum<FairMQStateMachine::Event> {};

} /* namespace std */

namespace fair
{
namespace mq
{
namespace fsm
{

// list of FSM states
struct OK_FSM_STATE                  : public state<> { static string Name() { return "OK"; }                  static FairMQStateMachine::State Type() { return FairMQStateMachine::State::OK; } };
struct ERROR_FSM_STATE               : public terminate_state<> { static string Name() { return "ERROR"; }     static FairMQStateMachine::State Type() { return FairMQStateMachine::State::Error; } };

struct IDLE_FSM_STATE                : public state<> { static string Name() { return "IDLE"; }                static FairMQStateMachine::State Type() { return FairMQStateMachine::State::IDLE; } };
struct INITIALIZING_DEVICE_FSM_STATE : public state<> { static string Name() { return "INITIALIZING_DEVICE"; } static FairMQStateMachine::State Type() { return FairMQStateMachine::State::INITIALIZING_DEVICE; } };
struct DEVICE_READY_FSM_STATE        : public state<> { static string Name() { return "DEVICE_READY"; }        static FairMQStateMachine::State Type() { return FairMQStateMachine::State::DEVICE_READY; } };
struct INITIALIZING_TASK_FSM_STATE   : public state<> { static string Name() { return "INITIALIZING_TASK"; }   static FairMQStateMachine::State Type() { return FairMQStateMachine::State::INITIALIZING_TASK; } };
struct READY_FSM_STATE               : public state<> { static string Name() { return "READY"; }               static FairMQStateMachine::State Type() { return FairMQStateMachine::State::READY; } };
struct RUNNING_FSM_STATE             : public state<> { static string Name() { return "RUNNING"; }             static FairMQStateMachine::State Type() { return FairMQStateMachine::State::RUNNING; } };
struct PAUSED_FSM_STATE              : public state<> { static string Name() { return "PAUSED"; }              static FairMQStateMachine::State Type() { return FairMQStateMachine::State::PAUSED; } };
struct RESETTING_TASK_FSM_STATE      : public state<> { static string Name() { return "RESETTING_TASK"; }      static FairMQStateMachine::State Type() { return FairMQStateMachine::State::RESETTING_TASK; } };
struct RESETTING_DEVICE_FSM_STATE    : public state<> { static string Name() { return "RESETTING_DEVICE"; }    static FairMQStateMachine::State Type() { return FairMQStateMachine::State::RESETTING_DEVICE; } };
struct EXITING_FSM_STATE             : public state<> { static string Name() { return "EXITING"; }             static FairMQStateMachine::State Type() { return FairMQStateMachine::State::EXITING; } };

// list of FSM events
struct INIT_DEVICE_FSM_EVENT           { static string Name() { return "INIT_DEVICE"; }           static FairMQStateMachine::Event Type() { return FairMQStateMachine::Event::INIT_DEVICE; } };
struct internal_DEVICE_READY_FSM_EVENT { static string Name() { return "internal_DEVICE_READY"; } static FairMQStateMachine::Event Type() { return FairMQStateMachine::Event::internal_DEVICE_READY; } };
struct INIT_TASK_FSM_EVENT             { static string Name() { return "INIT_TASK"; }             static FairMQStateMachine::Event Type() { return FairMQStateMachine::Event::INIT_TASK; } };
struct internal_READY_FSM_EVENT        { static string Name() { return "internal_READY"; }        static FairMQStateMachine::Event Type() { return FairMQStateMachine::Event::internal_READY; } };
struct RUN_FSM_EVENT                   { static string Name() { return "RUN"; }                   static FairMQStateMachine::Event Type() { return FairMQStateMachine::Event::RUN; } };
struct PAUSE_FSM_EVENT                 { static string Name() { return "PAUSE"; }                 static FairMQStateMachine::Event Type() { return FairMQStateMachine::Event::PAUSE; } };
struct STOP_FSM_EVENT                  { static string Name() { return "STOP"; }                  static FairMQStateMachine::Event Type() { return FairMQStateMachine::Event::STOP; } };
struct RESET_TASK_FSM_EVENT            { static string Name() { return "RESET_TASK"; }            static FairMQStateMachine::Event Type() { return FairMQStateMachine::Event::RESET_TASK; } };
struct RESET_DEVICE_FSM_EVENT          { static string Name() { return "RESET_DEVICE"; }          static FairMQStateMachine::Event Type() { return FairMQStateMachine::Event::RESET_DEVICE; } };
struct internal_IDLE_FSM_EVENT         { static string Name() { return "internal_IDLE"; }         static FairMQStateMachine::Event Type() { return FairMQStateMachine::Event::internal_IDLE; } };
struct END_FSM_EVENT                   { static string Name() { return "END"; }                   static FairMQStateMachine::Event Type() { return FairMQStateMachine::Event::END; } };
struct ERROR_FOUND_FSM_EVENT           { static string Name() { return "ERROR_FOUND"; }           static FairMQStateMachine::Event Type() { return FairMQStateMachine::Event::ERROR_FOUND; } };

static array<string, 12> stateNames =
{
    {
        "OK",
        "Error",
        "IDLE",
        "INITIALIZING_DEVICE",
        "DEVICE_READY",
        "INITIALIZING_TASK",
        "READY",
        "RUNNING",
        "PAUSED",
        "RESETTING_TASK",
        "RESETTING_DEVICE",
        "EXITING"
    }
};

static array<string, 12> eventNames =
{
    {
        "INIT_DEVICE",
        "internal_DEVICE_READY",
        "INIT_TASK",
        "internal_READY",
        "RUN",
        "PAUSE",
        "STOP",
        "RESET_TASK",
        "RESET_DEVICE",
        "internal_IDLE",
        "END",
        "ERROR_FOUND"
    }
};

static map<string, int> stateNumbers =
{
    { "OK",                  FairMQStateMachine::State::OK },
    { "Error",               FairMQStateMachine::State::Error },
    { "IDLE",                FairMQStateMachine::State::IDLE },
    { "INITIALIZING_DEVICE", FairMQStateMachine::State::INITIALIZING_DEVICE },
    { "DEVICE_READY",        FairMQStateMachine::State::DEVICE_READY },
    { "INITIALIZING_TASK",   FairMQStateMachine::State::INITIALIZING_TASK },
    { "READY",               FairMQStateMachine::State::READY },
    { "RUNNING",             FairMQStateMachine::State::RUNNING },
    { "PAUSED",              FairMQStateMachine::State::PAUSED },
    { "RESETTING_TASK",      FairMQStateMachine::State::RESETTING_TASK },
    { "RESETTING_DEVICE",    FairMQStateMachine::State::RESETTING_DEVICE },
    { "EXITING",             FairMQStateMachine::State::EXITING }
};

static map<string, int> eventNumbers =
{
    { "INIT_DEVICE",           FairMQStateMachine::Event::INIT_DEVICE },
    { "internal_DEVICE_READY", FairMQStateMachine::Event::internal_DEVICE_READY },
    { "INIT_TASK",             FairMQStateMachine::Event::INIT_TASK },
    { "internal_READY",        FairMQStateMachine::Event::internal_READY },
    { "RUN",                   FairMQStateMachine::Event::RUN },
    { "PAUSE",                 FairMQStateMachine::Event::PAUSE },
    { "STOP",                  FairMQStateMachine::Event::STOP },
    { "RESET_TASK",            FairMQStateMachine::Event::RESET_TASK },
    { "RESET_DEVICE",          FairMQStateMachine::Event::RESET_DEVICE },
    { "internal_IDLE",         FairMQStateMachine::Event::internal_IDLE },
    { "END",                   FairMQStateMachine::Event::END },
    { "ERROR_FOUND",           FairMQStateMachine::Event::ERROR_FOUND }
};

// defining the boost MSM state machine
struct Machine_ : public state_machine_def<Machine_>
{
  public:
    Machine_()
        : fUnblockHandler()
        , fStateHandlers()
        , fWork()
        , fWorkAvailableCondition()
        , fWorkDoneCondition()
        , fWorkMutex()
        , fWorkerTerminated(false)
        , fWorkActive(false)
        , fWorkAvailable(false)
        , fStateChangeSignal()
        , fStateChangeSignalsMap()
        , fState()
    {}

    virtual ~Machine_()
    {}

    // initial states
    using initial_state = boost::mpl::vector<IDLE_FSM_STATE, OK_FSM_STATE>;

    template<typename Event, typename FSM>
    void on_entry(Event const&, FSM& /*fsm*/)
    {
        LOG(state) << "Starting FairMQ state machine";
        fState = FairMQStateMachine::IDLE;
        LOG(state) << "Entering IDLE state";
        // fsm.CallStateChangeCallbacks(FairMQStateMachine::IDLE);
        // we call this for now in FairMQDevice::RunStateMachine()
    }

    template<typename Event, typename FSM>
    void on_exit(Event const&, FSM& /*fsm*/)
    {
        LOG(state) << "Exiting FairMQ state machine";
    }

    // actions
    struct AutomaticFct
    {
        template<typename EVT, typename FSM, typename SourceState, typename TargetState>
        void operator()(EVT const&, FSM& fsm, SourceState& /* ss */, TargetState& ts)
        {
            fsm.fState = ts.Type();
            LOG(state) << "Entering " << ts.Name() << " state";
        }
    };

    struct DefaultFct
    {
        template<typename EVT, typename FSM, typename SourceState, typename TargetState>
        void operator()(EVT const& e, FSM& fsm, SourceState& /* ss */, TargetState& ts)
        {
            fsm.fState = ts.Type();

            unique_lock<mutex> lock(fsm.fWorkMutex);
            while (fsm.fWorkActive)
            {
                fsm.fWorkDoneCondition.wait(lock);
            }
            fsm.fWorkAvailable = true;
            LOG(state) << "Entering " << ts.Name() << " state";
            fsm.fWork = fsm.fStateHandlers.at(e.Type());
            fsm.fWorkAvailableCondition.notify_one();
        }
    };

    struct PauseFct
    {
        template<typename EVT, typename FSM, typename SourceState, typename TargetState>
        void operator()(EVT const&, FSM& fsm, SourceState& /* ss */, TargetState& ts)
        {
            fsm.fState = ts.Type();

            fsm.fUnblockHandler();
            unique_lock<mutex> lock(fsm.fWorkMutex);
            while (fsm.fWorkActive)
            {
                fsm.fWorkDoneCondition.wait(lock);
            }
            fsm.fWorkAvailable = true;
            LOG(state) << "Entering " << ts.Name() << " state";
            fsm.fWork = fsm.fPauseWrapperHandler;
            fsm.fWorkAvailableCondition.notify_one();
        }
    };

    struct StopFct
    {
        template<typename EVT, typename FSM, typename SourceState, typename TargetState>
        void operator()(EVT const&, FSM& fsm, SourceState& /* ss */, TargetState& ts)
        {
            fsm.fState = ts.Type();

            fsm.fUnblockHandler();
            unique_lock<mutex> lock(fsm.fWorkMutex);
            while (fsm.fWorkActive)
            {
                fsm.fWorkDoneCondition.wait(lock);
            }
            LOG(state) << "Entering " << ts.Name() << " state";
        }
    };

    struct InternalStopFct
    {
        template<typename EVT, typename FSM, typename SourceState, typename TargetState>
        void operator()(EVT const&, FSM& fsm, SourceState& /* ss */, TargetState& ts)
        {
            fsm.fState = ts.Type();
            fsm.fUnblockHandler();
            LOG(state) << "RUNNING state finished without an external event, entering " << ts.Name() << " state";
        }
    };

    struct ExitingFct
    {
        template<typename EVT, typename FSM, typename SourceState, typename TargetState>
        void operator()(EVT const& e, FSM& fsm, SourceState& /* ss */, TargetState& ts)
        {
            LOG(state) << "Entering " << ts.Name() << " state";
            fsm.fState = ts.Type();
            fsm.CallStateChangeCallbacks(FairMQStateMachine::EXITING);

            // Stop ProcessWork()
            {
                lock_guard<mutex> lock(fsm.fWorkMutex);
                fsm.fWorkerTerminated = true;
                fsm.fWorkAvailableCondition.notify_one();
            }

            fsm.fStateHandlers.at(e.Type())();
        }
    };

    struct ErrorFoundFct
    {
        template<typename EVT, typename FSM, typename SourceState, typename TargetState>
        void operator()(EVT const&, FSM& fsm, SourceState& /* ss */, TargetState& ts)
        {
            fsm.fState = ts.Type();
            LOG(state) << "Entering " << ts.Name() << " state";
            fsm.CallStateChangeCallbacks(FairMQStateMachine::Error);
        }
    };

    // Transition table for Machine_
    struct transition_table : boost::mpl::vector<
        // Start                           Event                            Next                           Action           Guard
        Row<IDLE_FSM_STATE,                INIT_DEVICE_FSM_EVENT,           INITIALIZING_DEVICE_FSM_STATE, DefaultFct,      none>,
        Row<IDLE_FSM_STATE,                END_FSM_EVENT,                   EXITING_FSM_STATE,             ExitingFct,      none>,
        Row<INITIALIZING_DEVICE_FSM_STATE, internal_DEVICE_READY_FSM_EVENT, DEVICE_READY_FSM_STATE,        AutomaticFct,    none>,
        Row<DEVICE_READY_FSM_STATE,        INIT_TASK_FSM_EVENT,             INITIALIZING_TASK_FSM_STATE,   DefaultFct,      none>,
        Row<DEVICE_READY_FSM_STATE,        RESET_DEVICE_FSM_EVENT,          RESETTING_DEVICE_FSM_STATE,    DefaultFct,      none>,
        Row<INITIALIZING_TASK_FSM_STATE,   internal_READY_FSM_EVENT,        READY_FSM_STATE,               AutomaticFct,    none>,
        Row<READY_FSM_STATE,               RUN_FSM_EVENT,                   RUNNING_FSM_STATE,             DefaultFct,      none>,
        Row<READY_FSM_STATE,               RESET_TASK_FSM_EVENT,            RESETTING_TASK_FSM_STATE,      DefaultFct,      none>,
        Row<RUNNING_FSM_STATE,             PAUSE_FSM_EVENT,                 PAUSED_FSM_STATE,              DefaultFct,      none>,
        Row<RUNNING_FSM_STATE,             STOP_FSM_EVENT,                  READY_FSM_STATE,               StopFct,         none>,
        Row<RUNNING_FSM_STATE,             internal_READY_FSM_EVENT,        READY_FSM_STATE,               InternalStopFct, none>,
        Row<PAUSED_FSM_STATE,              RUN_FSM_EVENT,                   RUNNING_FSM_STATE,             DefaultFct,      none>,
        Row<RESETTING_TASK_FSM_STATE,      internal_DEVICE_READY_FSM_EVENT, DEVICE_READY_FSM_STATE,        AutomaticFct,    none>,
        Row<RESETTING_DEVICE_FSM_STATE,    internal_IDLE_FSM_EVENT,         IDLE_FSM_STATE,                AutomaticFct,    none>,
        Row<OK_FSM_STATE,                  ERROR_FOUND_FSM_EVENT,           ERROR_FSM_STATE,               ErrorFoundFct,   none>>
    {};

    // replaces the default no-transition response.
    template<typename FSM, typename Event>
    void no_transition(Event const& e, FSM&, int state)
    {
        using recursive_stt = typename boost::msm::back::recursive_get_transition_table<FSM>::type;
        using all_states = typename boost::msm::back::generate_state_set<recursive_stt>::type;

        string stateName;

        boost::mpl::for_each<all_states, boost::msm::wrap<boost::mpl::placeholders::_1>>(boost::msm::back::get_state_name<recursive_stt>(stateName, state));

        stateName = boost::core::demangle(stateName.c_str());
        size_t pos = stateName.rfind(":");
        if (pos != string::npos)
        {
            stateName = stateName.substr(pos + 1);
            stateName = stateName.substr(0, stateName.size() - 10);
        }

        if (stateName != "OK")
        {
            LOG(state) << "No transition from state " << stateName << " on event " << e.Name();
        }
    }

    void CallStateChangeCallbacks(const FairMQStateMachine::State state) const
    {
        if (!fStateChangeSignal.empty())
        {
            fStateChangeSignal(state);
        }
    }

    function<void(void)> fUnblockHandler;
    unordered_map<FairMQStateMachine::Event, function<void(void)>> fStateHandlers;

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

    atomic<FairMQStateMachine::State> fState;

    void ProcessWork()
    {
        while (true)
        {
            {
                unique_lock<mutex> lock(fWorkMutex);
                // Wait for work to be done.
                while (!fWorkAvailable && !fWorkerTerminated)
                {
                    fWorkAvailableCondition.wait_for(lock, chrono::milliseconds(100));
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
}; // Machine_

using FairMQFSM = boost::msm::back::state_machine<Machine_>;

} // namespace fsm
} // namespace mq
} // namespace fair

using namespace fair::mq::fsm;

FairMQStateMachine::FairMQStateMachine()
    : fChangeStateMutex()
    , fFsm(new FairMQFSM)
{
    static_pointer_cast<FairMQFSM>(fFsm)->fStateHandlers.emplace(INIT_DEVICE, bind(&FairMQStateMachine::InitWrapper, this));
    static_pointer_cast<FairMQFSM>(fFsm)->fStateHandlers.emplace(INIT_TASK, bind(&FairMQStateMachine::InitTaskWrapper, this));
    static_pointer_cast<FairMQFSM>(fFsm)->fStateHandlers.emplace(RUN, bind(&FairMQStateMachine::RunWrapper, this));
    static_pointer_cast<FairMQFSM>(fFsm)->fStateHandlers.emplace(PAUSE, bind(&FairMQStateMachine::PauseWrapper, this));
    static_pointer_cast<FairMQFSM>(fFsm)->fStateHandlers.emplace(RESET_TASK, bind(&FairMQStateMachine::ResetTaskWrapper, this));
    static_pointer_cast<FairMQFSM>(fFsm)->fStateHandlers.emplace(RESET_DEVICE, bind(&FairMQStateMachine::ResetWrapper, this));
    static_pointer_cast<FairMQFSM>(fFsm)->fStateHandlers.emplace(END, bind(&FairMQStateMachine::Exit, this));
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
                static_pointer_cast<FairMQFSM>(fFsm)->process_event(INIT_DEVICE_FSM_EVENT());
                return true;
            }
            case internal_DEVICE_READY:
            {
                lock_guard<mutex> lock(fChangeStateMutex);
                static_pointer_cast<FairMQFSM>(fFsm)->process_event(internal_DEVICE_READY_FSM_EVENT());
                return true;
            }
            case INIT_TASK:
            {
                lock_guard<mutex> lock(fChangeStateMutex);
                static_pointer_cast<FairMQFSM>(fFsm)->process_event(INIT_TASK_FSM_EVENT());
                return true;
            }
            case internal_READY:
            {
                lock_guard<mutex> lock(fChangeStateMutex);
                static_pointer_cast<FairMQFSM>(fFsm)->process_event(internal_READY_FSM_EVENT());
                return true;
            }
            case RUN:
            {
                lock_guard<mutex> lock(fChangeStateMutex);
                static_pointer_cast<FairMQFSM>(fFsm)->process_event(RUN_FSM_EVENT());
                return true;
            }
            case PAUSE:
            {
                lock_guard<mutex> lock(fChangeStateMutex);
                static_pointer_cast<FairMQFSM>(fFsm)->process_event(PAUSE_FSM_EVENT());
                return true;
            }
            case STOP:
            {
                lock_guard<mutex> lock(fChangeStateMutex);
                static_pointer_cast<FairMQFSM>(fFsm)->process_event(STOP_FSM_EVENT());
                return true;
            }
            case RESET_DEVICE:
            {
                lock_guard<mutex> lock(fChangeStateMutex);
                static_pointer_cast<FairMQFSM>(fFsm)->process_event(RESET_DEVICE_FSM_EVENT());
                return true;
            }
            case RESET_TASK:
            {
                lock_guard<mutex> lock(fChangeStateMutex);
                static_pointer_cast<FairMQFSM>(fFsm)->process_event(RESET_TASK_FSM_EVENT());
                return true;
            }
            case internal_IDLE:
            {
                lock_guard<mutex> lock(fChangeStateMutex);
                static_pointer_cast<FairMQFSM>(fFsm)->process_event(internal_IDLE_FSM_EVENT());
                return true;
            }
            case END:
            {
                lock_guard<mutex> lock(fChangeStateMutex);
                static_pointer_cast<FairMQFSM>(fFsm)->process_event(END_FSM_EVENT());
                return true;
            }
            case ERROR_FOUND:
            {
                lock_guard<mutex> lock(fChangeStateMutex);
                static_pointer_cast<FairMQFSM>(fFsm)->process_event(ERROR_FOUND_FSM_EVENT());
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
    return GetStateName(static_pointer_cast<FairMQFSM>(fFsm)->fState);
}
string FairMQStateMachine::GetStateName(const State state)
{
    return stateNames.at(state);
}
int FairMQStateMachine::GetCurrentState() const
{
    return static_pointer_cast<FairMQFSM>(fFsm)->fState;
}
bool FairMQStateMachine::CheckCurrentState(int state) const
{
    return state == static_pointer_cast<FairMQFSM>(fFsm)->fState;
}
bool FairMQStateMachine::CheckCurrentState(const string& state) const
{
    return state == GetCurrentStateName();
}

void FairMQStateMachine::ProcessWork()
try
{
    static_pointer_cast<FairMQFSM>(fFsm)->ProcessWork();
} catch(...) {
    {
        lock_guard<mutex> lock(static_pointer_cast<FairMQFSM>(fFsm)->fWorkMutex);
        static_pointer_cast<FairMQFSM>(fFsm)->fWorkActive = false;
        static_pointer_cast<FairMQFSM>(fFsm)->fWorkAvailable = false;
        static_pointer_cast<FairMQFSM>(fFsm)->fWorkDoneCondition.notify_one();
    }
    ChangeState(ERROR_FOUND);
    throw;
}


int FairMQStateMachine::GetEventNumber(const string& event)
{
    return eventNumbers.at(event);
}
