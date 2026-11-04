/********************************************************************************
 *    Copyright (C) 2026 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <fairmq/StateMachine.h>
#include <fairmq/States.h>

#include <helper/Sync.h>

#include <gtest/gtest.h>

#include <atomic>
#include <map>
#include <mutex>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{

using namespace std;
using namespace fair::mq;

auto AllStates() -> vector<State>
{
    return {State::Undefined, State::Ok, State::Error, State::Idle, State::InitializingDevice,
            State::Initialized, State::Binding, State::Bound, State::Connecting, State::DeviceReady,
            State::InitializingTask, State::Ready, State::Running, State::ResettingTask,
            State::ResettingDevice, State::Exiting};
}

auto AllTransitions() -> vector<Transition>
{
    return {Transition::Auto, Transition::InitDevice, Transition::CompleteInit, Transition::Bind,
            Transition::Connect, Transition::InitTask, Transition::Run, Transition::Stop,
            Transition::ResetTask, Transition::ResetDevice, Transition::End, Transition::ErrorFound};
}

/// A started state machine with ProcessWork() in a thread.
class Machine
{
  public:
    Machine()
    {
        fFsm.SubscribeToStateChange("test", [this](State state) {
            std::lock_guard<std::mutex> lock(fMtx);
            fStates.push_back(state);
        });
        fFsm.Start();
        fWorker = std::make_unique<test::JoiningThread>([this] {
            try {
                fFsm.ProcessWork();
            } catch (StateMachine::ErrorStateException const&) {
                fErrorStateException = true;
            }
        });
        // ProcessWork() reports Idle first. Wait for it, so that later
        // subscriptions see only the changes that the test makes.
        test::WaitUntil([this] { return !States().empty(); });
    }

    Machine(const Machine&) = delete;
    Machine(Machine&&) = delete;
    Machine& operator=(const Machine&) = delete;
    Machine& operator=(Machine&&) = delete;

    ~Machine()
    {
        // End ProcessWork(): go to Exiting or Error, whichever is reachable.
        if (!fFsm.ChangeState(Transition::End)) {
            fFsm.ChangeState(Transition::ErrorFound);
        }
    }

    /// Change the state and wait until ProcessWork() has handled the new state.
    auto Change(Transition transition, State expected) -> testing::AssertionResult
    {
        if (!fFsm.ChangeState(transition)) {
            return testing::AssertionFailure() << transition << " refused in " << fFsm.GetCurrentState();
        }
        if (!test::WaitUntil([&] { return fFsm.GetCurrentState() == expected && !fFsm.NewStatePending(); })) {
            return testing::AssertionFailure() << transition << " gave " << fFsm.GetCurrentState()
                                               << ", expected " << expected;
        }
        return testing::AssertionSuccess();
    }

    auto States() -> vector<State>
    {
        std::lock_guard<std::mutex> lock(fMtx);
        return fStates;
    }

    StateMachine fFsm;
    std::atomic<bool> fErrorStateException{false};

  private:
    std::mutex fMtx;
    vector<State> fStates;
    std::unique_ptr<test::JoiningThread> fWorker;
};

struct Step
{
    State state;            // the state before the transition
    Transition transition;  // the legal transition that the test takes
    State next;             // the state after the transition
};

// Each state of the device lifecycle once, with a legal transition out of it.
auto const kLifecycle = vector<Step>{
    {State::Idle,               Transition::InitDevice,   State::InitializingDevice},
    {State::InitializingDevice, Transition::CompleteInit, State::Initialized},
    {State::Initialized,        Transition::Bind,         State::Binding},
    {State::Binding,            Transition::Auto,         State::Bound},
    {State::Bound,              Transition::Connect,      State::Connecting},
    {State::Connecting,         Transition::Auto,         State::DeviceReady},
    {State::DeviceReady,        Transition::InitTask,     State::InitializingTask},
    {State::InitializingTask,   Transition::Auto,         State::Ready},
    {State::Ready,              Transition::Run,          State::Running},
    {State::Running,            Transition::Stop,         State::Ready},
    {State::Ready,              Transition::ResetTask,    State::ResettingTask},
    {State::ResettingTask,      Transition::Auto,         State::DeviceReady},
    {State::DeviceReady,        Transition::ResetDevice,  State::ResettingDevice},
    {State::ResettingDevice,    Transition::Auto,         State::Idle},
    {State::Idle,               Transition::End,          State::Exiting},
};

// The legal transitions from each state, other than ErrorFound (legal in all states).
auto const kLegal = map<State, set<Transition>>{
    {State::Idle,               {Transition::InitDevice, Transition::End}},
    {State::InitializingDevice, {Transition::CompleteInit}},
    {State::Initialized,        {Transition::Bind, Transition::ResetDevice}},
    {State::Binding,            {Transition::Auto}},
    {State::Bound,              {Transition::Connect, Transition::ResetDevice}},
    {State::Connecting,         {Transition::Auto}},
    {State::DeviceReady,        {Transition::InitTask, Transition::ResetDevice}},
    {State::InitializingTask,   {Transition::Auto}},
    {State::Ready,              {Transition::Run, Transition::ResetTask}},
    {State::Running,            {Transition::Stop}},
    {State::ResettingTask,      {Transition::Auto}},
    {State::ResettingDevice,    {Transition::Auto}},
};

TEST(StateMachine, StartsInIdle)
{
    Machine m;
    ASSERT_TRUE(test::WaitUntil([&] { return m.fFsm.GetCurrentState() == State::Idle; }));
    EXPECT_EQ(m.fFsm.GetCurrentStateName(), "IDLE");
    EXPECT_FALSE(m.fFsm.NewStatePending());
}

TEST(StateMachine, RefusesIllegalTransitions)
{
    Machine m;
    for (auto const& step : kLifecycle) {
        ASSERT_EQ(m.fFsm.GetCurrentState(), step.state);
        for (auto const transition : AllTransitions()) {
            if (transition == Transition::ErrorFound || kLegal.at(step.state).count(transition) > 0) {
                continue;
            }
            EXPECT_FALSE(m.fFsm.ChangeState(transition)) << transition << " in " << step.state;
            EXPECT_FALSE(m.fFsm.NewStatePending()) << transition << " in " << step.state;
            EXPECT_EQ(m.fFsm.GetCurrentState(), step.state) << transition << " in " << step.state;
        }
        ASSERT_TRUE(m.Change(step.transition, step.next));
    }
}

TEST(StateMachine, ResetDeviceFromEachDeviceState)
{
    for (auto const state : {State::Initialized, State::Bound, State::DeviceReady}) {
        Machine m;
        for (auto const& step : kLifecycle) {
            if (m.fFsm.GetCurrentState() == state) {
                break;
            }
            ASSERT_TRUE(m.Change(step.transition, step.next));
        }
        ASSERT_TRUE(m.Change(Transition::ResetDevice, State::ResettingDevice)) << "from " << state;
        ASSERT_TRUE(m.Change(Transition::Auto, State::Idle)) << "from " << state;
    }
}

TEST(StateMachine, RefusesTransitionWhileAnotherIsPending)
{
    StateMachine fsm;
    fsm.Start();
    // Without ProcessWork() the new state stays pending.
    ASSERT_TRUE(fsm.ChangeState(Transition::InitDevice));
    EXPECT_TRUE(fsm.NewStatePending());
    EXPECT_FALSE(fsm.ChangeState(Transition::CompleteInit));
    EXPECT_FALSE(fsm.ChangeState(Transition::End));
}

TEST(StateMachine, WaitForPendingState)
{
    StateMachine fsm;
    fsm.Start();
    EXPECT_FALSE(fsm.WaitForPendingStateFor(10));
    ASSERT_TRUE(fsm.ChangeState(Transition::InitDevice));
    EXPECT_TRUE(fsm.WaitForPendingStateFor(10));
    fsm.WaitForPendingState(); // returns at once
}

TEST(StateMachine, ChangeStateByName)
{
    Machine m;
    EXPECT_TRUE(m.fFsm.ChangeState("INIT DEVICE"));
    ASSERT_TRUE(test::WaitUntil([&] { return m.fFsm.GetCurrentState() == State::InitializingDevice; }));
    EXPECT_THROW(m.fFsm.ChangeState("NO SUCH TRANSITION"), std::out_of_range);
}

TEST(StateMachine, ErrorFoundFromEachState)
{
    for (size_t n = 0; n < kLifecycle.size(); ++n) {
        Machine m;
        for (size_t i = 0; i < n; ++i) {
            ASSERT_TRUE(m.Change(kLifecycle.at(i).transition, kLifecycle.at(i).next));
        }
        auto const from = m.fFsm.GetCurrentState();
        ASSERT_TRUE(m.Change(Transition::ErrorFound, State::Error)) << "from " << from;
        EXPECT_TRUE(test::WaitUntil([&] { return m.fErrorStateException.load(); })) << "from " << from;
    }
}

// Error is a terminate state: the state machine refuses all transitions in it.
// ErrorFound succeeds, because the device is already in the Error state.
TEST(StateMachine, RefusesTransitionsInError)
{
    Machine m;
    ASSERT_TRUE(m.Change(Transition::ErrorFound, State::Error));
    for (auto const transition : AllTransitions()) {
        EXPECT_EQ(m.fFsm.ChangeState(transition), transition == Transition::ErrorFound) << transition << " in Error";
    }
}

TEST(StateMachine, StateChangeSubscription)
{
    vector<State> expected{State::Idle};
    {
        Machine m;
        for (auto const& step : kLifecycle) {
            ASSERT_TRUE(m.Change(step.transition, step.next));
            expected.push_back(step.next);
        }
        ASSERT_TRUE(test::WaitUntil([&] { return m.States().size() == expected.size(); }));
        EXPECT_EQ(m.States(), expected);
    }
}

TEST(StateMachine, TransitionSubscription)
{
    Machine m;
    std::mutex mtx;
    vector<Transition> transitions;
    m.fFsm.SubscribeToNewTransition("test", [&](Transition t) {
        std::lock_guard<std::mutex> lock(mtx);
        transitions.push_back(t);
    });
    ASSERT_TRUE(m.Change(Transition::InitDevice, State::InitializingDevice));
    EXPECT_FALSE(m.fFsm.ChangeState(Transition::Run)); // refused: no callback
    ASSERT_TRUE(m.Change(Transition::CompleteInit, State::Initialized));
    m.fFsm.UnsubscribeFromNewTransition("test");
    ASSERT_TRUE(m.Change(Transition::ResetDevice, State::ResettingDevice));

    std::lock_guard<std::mutex> lock(mtx);
    EXPECT_EQ(transitions, (vector<Transition>{Transition::InitDevice, Transition::CompleteInit}));
}

TEST(StateMachine, UnsubscribeFromStateChange)
{
    Machine m;
    std::atomic<int> calls{0};
    m.fFsm.SubscribeToStateChange("counter", [&](State) { ++calls; });
    ASSERT_TRUE(m.Change(Transition::InitDevice, State::InitializingDevice));
    m.fFsm.UnsubscribeFromStateChange("counter");
    m.fFsm.UnsubscribeFromStateChange("no such key"); // no effect
    ASSERT_TRUE(m.Change(Transition::CompleteInit, State::Initialized));
    EXPECT_EQ(calls, 1);
}

TEST(StateMachine, StateHandlers)
{
    StateMachine fsm;
    std::mutex mtx;
    vector<string> calls;
    fsm.PrepareState([&](State s) { std::lock_guard<std::mutex> l(mtx); calls.push_back("prep " + GetStateName(s)); });
    fsm.HandleStates([&](State s) {
        {
            std::lock_guard<std::mutex> l(mtx);
            calls.push_back("handle " + GetStateName(s));
        }
        // The handler completes the transient states, as the Device does.
        if (s == State::InitializingDevice) {
            fsm.ChangeState(Transition::CompleteInit);
        } else if (s == State::Initialized) {
            fsm.ChangeState(Transition::ResetDevice);
        } else if (s == State::ResettingDevice) {
            fsm.ChangeState(Transition::Auto);
        } else if (s == State::Idle) {
            fsm.ChangeState(Transition::End);
        }
    });
    fsm.Start();
    ASSERT_TRUE(fsm.ChangeState(Transition::InitDevice));
    test::JoiningThread worker([&] { fsm.ProcessWork(); });
    worker.Join();

    EXPECT_EQ(fsm.GetCurrentState(), State::Exiting);
    EXPECT_EQ(calls, (vector<string>{"prep INITIALIZING DEVICE", "handle INITIALIZING DEVICE",
                                     "prep INITIALIZED", "handle INITIALIZED",
                                     "prep RESETTING DEVICE", "handle RESETTING DEVICE",
                                     "prep IDLE", "handle IDLE",
                                     "prep EXITING", "handle EXITING"}));

    fsm.StopHandlingStates();
}

TEST(States, NameRoundTrip)
{
    for (auto const state : AllStates()) {
        EXPECT_EQ(GetState(GetStateName(state)), state) << GetStateName(state);
    }
    for (auto const transition : AllTransitions()) {
        EXPECT_EQ(GetTransition(GetTransitionName(transition)), transition) << GetTransitionName(transition);
    }
}

TEST(States, UnknownNameThrows)
{
    EXPECT_THROW(GetState("NO SUCH STATE"), std::out_of_range);
    EXPECT_THROW(GetTransition("NO SUCH TRANSITION"), std::out_of_range);
    EXPECT_THROW(GetStateName(static_cast<State>(100)), std::out_of_range);
    EXPECT_THROW(GetTransitionName(static_cast<Transition>(100)), std::out_of_range);
}

} // namespace
