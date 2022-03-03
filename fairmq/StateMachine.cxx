/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <fairmq/StateMachine.h>
#include <fairmq/tools/Exceptions.h>

#include <fairlogger/Logger.h>

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
#include <unordered_map>
#include <mutex>

using namespace std;
using namespace boost::msm;
using namespace boost::msm::front;
using namespace boost::msm::back;
namespace bmpl = boost::mpl;

namespace fair::mq
{
namespace fsm
{

// list of FSM states
struct OK_S                  : public state<> { static string Name() { return "OK"; }                  static State Type() { return State::Ok; } };

struct IDLE_S                : public state<> { static string Name() { return "IDLE"; }                static State Type() { return State::Idle; } };
struct INITIALIZING_DEVICE_S : public state<> { static string Name() { return "INITIALIZING_DEVICE"; } static State Type() { return State::InitializingDevice; } };
struct INITIALIZED_S         : public state<> { static string Name() { return "INITIALIZED"; }         static State Type() { return State::Initialized; } };
struct BINDING_S             : public state<> { static string Name() { return "BINDING"; }             static State Type() { return State::Binding; } };
struct BOUND_S               : public state<> { static string Name() { return "BOUND"; }               static State Type() { return State::Bound; } };
struct CONNECTING_S          : public state<> { static string Name() { return "CONNECTING"; }          static State Type() { return State::Connecting; } };
struct DEVICE_READY_S        : public state<> { static string Name() { return "DEVICE_READY"; }        static State Type() { return State::DeviceReady; } };
struct INITIALIZING_TASK_S   : public state<> { static string Name() { return "INITIALIZING_TASK"; }   static State Type() { return State::InitializingTask; } };
struct READY_S               : public state<> { static string Name() { return "READY"; }               static State Type() { return State::Ready; } };
struct RUNNING_S             : public state<> { static string Name() { return "RUNNING"; }             static State Type() { return State::Running; } };
struct RESETTING_TASK_S      : public state<> { static string Name() { return "RESETTING_TASK"; }      static State Type() { return State::ResettingTask; } };
struct RESETTING_DEVICE_S    : public state<> { static string Name() { return "RESETTING_DEVICE"; }    static State Type() { return State::ResettingDevice; } };
struct EXITING_S             : public state<> { static string Name() { return "EXITING"; }             static State Type() { return State::Exiting; } };

struct ERROR_S               : public terminate_state<> { static string Name() { return "ERROR"; }     static State Type() { return State::Error; } };

// list of FSM transitions (events)
struct AUTO_E          { static string Name() { return "AUTO"; }          static Transition Type() { return Transition::Auto; } };
struct INIT_DEVICE_E   { static string Name() { return "INIT_DEVICE"; }   static Transition Type() { return Transition::InitDevice; } };
struct COMPLETE_INIT_E { static string Name() { return "COMPLETE_INIT"; } static Transition Type() { return Transition::CompleteInit; } };
struct BIND_E          { static string Name() { return "BIND"; }          static Transition Type() { return Transition::Bind; } };
struct CONNECT_E       { static string Name() { return "CONNECT"; }       static Transition Type() { return Transition::Connect; } };
struct INIT_TASK_E     { static string Name() { return "INIT_TASK"; }     static Transition Type() { return Transition::InitTask; } };
struct RUN_E           { static string Name() { return "RUN"; }           static Transition Type() { return Transition::Run; } };
struct STOP_E          { static string Name() { return "STOP"; }          static Transition Type() { return Transition::Stop; } };
struct RESET_TASK_E    { static string Name() { return "RESET_TASK"; }    static Transition Type() { return Transition::ResetTask; } };
struct RESET_DEVICE_E  { static string Name() { return "RESET_DEVICE"; }  static Transition Type() { return Transition::ResetDevice; } };
struct END_E           { static string Name() { return "END"; }           static Transition Type() { return Transition::End; } };
struct ERROR_FOUND_E   { static string Name() { return "ERROR_FOUND"; }   static Transition Type() { return Transition::ErrorFound; } };

// defining the boost MSM state machine
struct Machine_ : public state_machine_def<Machine_>
{
  public:
    Machine_()
        : fState(State::Ok)
        , fNewState(State::Ok)
        , fLastTransitionResult(true)
        , fNewStatePending(false)
    {}

    // initial states
    using initial_state = bmpl::vector<IDLE_S, OK_S>;

    template<typename Transition, typename FSM>
    void on_entry(Transition const&, FSM& /* fsm */)
    {
        LOG(state) << "Starting FairMQ state machine --> IDLE";
        fState = State::Idle;
    }

    template<typename Transition, typename FSM>
    void on_exit(Transition const&, FSM& /*fsm*/)
    {
        LOG(state) << "Exiting FairMQ state machine";
    }

    struct DefaultFct
    {
        template<typename EVT, typename FSM, typename SourceState, typename TargetState>
        void operator()(EVT const& e, FSM& fsm, SourceState& /* ss */, TargetState& ts)
        {
            fsm.fNewState = ts.Type();
            fsm.fLastTransitionResult = true;
            fsm.CallNewTransitionCallbacks(e.Type());
            fsm.fNewStatePending = true;
            fsm.fNewStatePendingCV.notify_all();
        }
    };

    struct transition_table : bmpl::vector<
        //  Start                  Transition       Next                   Action      Guard
        Row<IDLE_S,                END_E,           EXITING_S,             DefaultFct, none>,
        Row<IDLE_S,                INIT_DEVICE_E,   INITIALIZING_DEVICE_S, DefaultFct, none>,

        Row<INITIALIZING_DEVICE_S, COMPLETE_INIT_E, INITIALIZED_S,         DefaultFct, none>,
        Row<INITIALIZED_S,         BIND_E,          BINDING_S,             DefaultFct, none>,
        Row<INITIALIZED_S,         RESET_DEVICE_E,  RESETTING_DEVICE_S,    DefaultFct, none>,

        Row<BINDING_S,             AUTO_E,          BOUND_S,               DefaultFct, none>,
        Row<BOUND_S,               CONNECT_E,       CONNECTING_S,          DefaultFct, none>,
        Row<BOUND_S,               RESET_DEVICE_E,  RESETTING_DEVICE_S,    DefaultFct, none>,

        Row<CONNECTING_S,          AUTO_E,          DEVICE_READY_S,        DefaultFct, none>,
        Row<DEVICE_READY_S,        INIT_TASK_E,     INITIALIZING_TASK_S,   DefaultFct, none>,
        Row<DEVICE_READY_S,        RESET_DEVICE_E,  RESETTING_DEVICE_S,    DefaultFct, none>,

        Row<INITIALIZING_TASK_S,   AUTO_E,          READY_S,               DefaultFct, none>,

        Row<READY_S,               RUN_E,           RUNNING_S,             DefaultFct, none>,
        Row<READY_S,               RESET_TASK_E,    RESETTING_TASK_S,      DefaultFct, none>,

        Row<RUNNING_S,             STOP_E,          READY_S,               DefaultFct, none>,

        Row<RESETTING_TASK_S,      AUTO_E,          DEVICE_READY_S,        DefaultFct, none>,
        Row<RESETTING_DEVICE_S,    AUTO_E,          IDLE_S,                DefaultFct, none>,

        Row<OK_S,                  ERROR_FOUND_E,   ERROR_S,               DefaultFct, none>> {};

    void CallStateChangeCallbacks(const State state) const
    {
        if (!fStateChangeSignal.empty()) {
            fStateChangeSignal(state);
        }
    }

    void CallStateHandler(const State state) const
    {
        if (!fStateHandleSignal.empty()) {
            fStateHandleSignal(state);
        }
    }

    void CallNewTransitionCallbacks(const Transition transition) const
    {
        if (!fNewTransitionSignal.empty()) {
            fNewTransitionSignal(transition);
        }
    }


    atomic<State> fState;
    atomic<State> fNewState;
    atomic<bool> fLastTransitionResult;

    mutex fStateMtx;
    atomic<bool> fNewStatePending;
    condition_variable fNewStatePendingCV;

    boost::signals2::signal<void(const State)> fStateChangeSignal;
    boost::signals2::signal<void(const State)> fStateHandleSignal;
    boost::signals2::signal<void(const Transition)> fNewTransitionSignal;
    unordered_map<string, boost::signals2::connection> fStateChangeSignalsMap;
    unordered_map<string, boost::signals2::connection> fNewTransitionSignalsMap;

    void ProcessWork()
    {
        bool stop = false;

        while (!stop) {
            {
                unique_lock<mutex> lock(fStateMtx);

                fNewStatePendingCV.wait(lock, [this]{ return fNewStatePending.load(); });

                LOG(state) << fState << " ---> " << fNewState;
                fState = static_cast<State>(fNewState);
                fNewStatePending = false;

                if (fState == State::Exiting || fState == State::Error) {
                    stop = true;
                }
            }

            CallStateChangeCallbacks(fState);
            CallStateHandler(fState);
        }

        if (fState == State::Error) {
            LOG(trace) << "Device transitioned to error state";
            throw StateMachine::ErrorStateException("Device transitioned to error state");
        }
    }

    // replaces the default no-transition response.
    template<typename FSM, typename Transition>
    void no_transition(Transition const& t, FSM& fsm, int state)
    {
        using RecursiveStt = typename recursive_get_transition_table<FSM>::type;
        using AllStates = typename generate_state_set<RecursiveStt>::type;

        string stateName;

        bmpl::for_each<AllStates, wrap<bmpl::placeholders::_1>>(get_state_name<RecursiveStt>(stateName, state));

        stateName = boost::core::demangle(stateName.c_str());
        size_t pos = stateName.rfind(':');
        stateName = stateName.substr(pos + 1);
        size_t pos2 = stateName.rfind('_');
        stateName = stateName.substr(0, pos2);

        if (stateName != "OK") {
            LOG(state) << "No transition from state " << stateName << " on transition " << t.Name();
        }
        fsm.fLastTransitionResult = false;
    }
}; // Machine_

using FairMQFSM = state_machine<Machine_>;

} // namespace fsm
} // namespace fair::mq

using namespace fair::mq::fsm;
using namespace fair::mq;

StateMachine::StateMachine() : fFsm(new FairMQFSM) {}
void StateMachine::Start() { static_pointer_cast<FairMQFSM>(fFsm)->start(); }
StateMachine::~StateMachine() { static_pointer_cast<FairMQFSM>(fFsm)->stop(); }

bool StateMachine::ChangeState(Transition transition)
try {
    auto fsm = static_pointer_cast<FairMQFSM>(fFsm);
    lock_guard<mutex> lock(fsm->fStateMtx);
    if (!static_cast<bool>(fsm->fNewStatePending) || transition == Transition::ErrorFound) {
        switch (transition) {
            case Transition::Auto:
                fsm->process_event(AUTO_E());
                return fsm->fLastTransitionResult;
            case Transition::InitDevice:
                fsm->process_event(INIT_DEVICE_E());
                return fsm->fLastTransitionResult;
            case Transition::CompleteInit:
                fsm->process_event(COMPLETE_INIT_E());
                return fsm->fLastTransitionResult;
            case Transition::Bind:
                fsm->process_event(BIND_E());
                return fsm->fLastTransitionResult;
            case Transition::Connect:
                fsm->process_event(CONNECT_E());
                return fsm->fLastTransitionResult;
            case Transition::InitTask:
                fsm->process_event(INIT_TASK_E());
                return fsm->fLastTransitionResult;
            case Transition::Run:
                fsm->process_event(RUN_E());
                return fsm->fLastTransitionResult;
            case Transition::Stop:
                fsm->process_event(STOP_E());
                return fsm->fLastTransitionResult;
            case Transition::ResetDevice:
                fsm->process_event(RESET_DEVICE_E());
                return fsm->fLastTransitionResult;
            case Transition::ResetTask:
                fsm->process_event(RESET_TASK_E());
                return fsm->fLastTransitionResult;
            case Transition::End:
                fsm->process_event(END_E());
                return fsm->fLastTransitionResult;
            case Transition::ErrorFound:
                fsm->process_event(ERROR_FOUND_E());
                return fsm->fLastTransitionResult;
            default:
                LOG(error) << "Requested unsupported state transition: " << transition << endl;
                return false;
             }
    } else {
        LOG(state) << "Transition " << GetTransitionName(transition) << " incoming, but another state transition is already ongoing.";
        return false;
    }
} catch (exception& e) {
    LOG(error) << "Exception in StateMachine::ChangeState(): " << e.what();
    return false;
}

void StateMachine::SubscribeToStateChange(const string& key, function<void(const State)> callback)
{
    static_pointer_cast<FairMQFSM>(fFsm)->fStateChangeSignalsMap.insert({key, static_pointer_cast<FairMQFSM>(fFsm)->fStateChangeSignal.connect(callback)});
}

void StateMachine::UnsubscribeFromStateChange(const string& key)
{
    auto fsm = static_pointer_cast<FairMQFSM>(fFsm);
    if (fsm->fStateChangeSignalsMap.count(key)) {
        fsm->fStateChangeSignalsMap.at(key).disconnect();
        fsm->fStateChangeSignalsMap.erase(key);
    }
}

void StateMachine::HandleStates(function<void(const State)> callback)
{
    auto fsm = static_pointer_cast<FairMQFSM>(fFsm);
    if (fsm->fStateHandleSignal.empty()) {
        fsm->fStateHandleSignal.connect(callback);
    } else {
        LOG(error) << "state handler is already set";
    }
}

void StateMachine::StopHandlingStates()
{
    auto fsm = static_pointer_cast<FairMQFSM>(fFsm);
    if (!fsm->fStateHandleSignal.empty()) {
        fsm->fStateHandleSignal.disconnect_all_slots();
    }
}

void StateMachine::SubscribeToNewTransition(const string& key, function<void(const Transition)> callback)
{
    static_pointer_cast<FairMQFSM>(fFsm)->fNewTransitionSignalsMap.insert({key, static_pointer_cast<FairMQFSM>(fFsm)->fNewTransitionSignal.connect(callback)});
}

void StateMachine::UnsubscribeFromNewTransition(const string& key)
{
    auto fsm = static_pointer_cast<FairMQFSM>(fFsm);
    if (fsm->fNewTransitionSignalsMap.count(key)) {
        fsm->fNewTransitionSignalsMap.at(key).disconnect();
        fsm->fNewTransitionSignalsMap.erase(key);
    }
}

State StateMachine::GetCurrentState() const { return static_pointer_cast<FairMQFSM>(fFsm)->fState; }
string StateMachine::GetCurrentStateName() const { return GetStateName(static_pointer_cast<FairMQFSM>(fFsm)->fState); }

bool StateMachine::NewStatePending() const { return static_cast<bool>(static_pointer_cast<FairMQFSM>(fFsm)->fNewStatePending); }
void StateMachine::WaitForPendingState() const
{
    auto fsm = static_pointer_cast<FairMQFSM>(fFsm);
    unique_lock<mutex> lock(fsm->fStateMtx);
    fsm->fNewStatePendingCV.wait(lock, [&]{ return static_cast<bool>(fsm->fNewStatePending); });
}
bool StateMachine::WaitForPendingStateFor(int durationInMs) const
{
    auto fsm = static_pointer_cast<FairMQFSM>(fFsm);
    unique_lock<mutex> lock(fsm->fStateMtx);
    return fsm->fNewStatePendingCV.wait_for(lock, std::chrono::milliseconds(durationInMs), [&]{ return static_cast<bool>(fsm->fNewStatePending); });
}

void StateMachine::ProcessWork()
{
    auto fsm = static_pointer_cast<FairMQFSM>(fFsm);

    fair::mq::tools::CallOnDestruction cod([&](){
        LOG(debug) << "Exception caught in ProcessWork(), going to Error state";
        {
            lock_guard<mutex> lock(fsm->fStateMtx);
            fsm->fState = State::Error;
            fsm->CallStateChangeCallbacks(State::Error);
        }
        ChangeState(Transition::ErrorFound);
    });

    fsm->CallStateChangeCallbacks(State::Idle);
    fsm->ProcessWork();

    cod.disable();
}
