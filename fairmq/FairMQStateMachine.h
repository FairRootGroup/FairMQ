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

#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/msm/back/state_machine.hpp>
#include <boost/msm/front/state_machine_def.hpp>
#include <boost/msm/front/functor_row.hpp>
#include <boost/msm/front/euml/common.hpp>
#include <boost/msm/front/euml/operator.hpp>
#include <boost/function.hpp>

#include "FairMQLogger.h"

namespace msm = boost::msm;
namespace mpl = boost::mpl;
namespace msmf = boost::msm::front;

namespace FairMQFSM
{
    // defining events for the boost MSM state machine
    struct INIT
    {
    };
    struct SETOUTPUT
    {
    };
    struct SETINPUT
    {
    };
    struct PAUSE
    {
    };
    struct RUN
    {
    };
    struct STOP
    {
    };
    struct END
    {
    };

    // defining the boost MSM state machine
    struct FairMQFSM_ : public msm::front::state_machine_def<FairMQFSM_>
    {
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
        struct IDLE_FSM : public msm::front::state<>
        {
        };
        struct INITIALIZING_FSM : public msm::front::state<>
        {
        };
        struct SETTINGOUTPUT_FSM : public msm::front::state<>
        {
        };
        struct SETTINGINPUT_FSM : public msm::front::state<>
        {
        };
        struct WAITING_FSM : public msm::front::state<>
        {
        };
        struct RUNNING_FSM : public msm::front::state<>
        {
        };
        // Define initial state
        typedef IDLE_FSM initial_state;
        // Actions
        struct TestFct
        {
            template <class EVT, class FSM, class SourceState, class TargetState>
            void operator()(EVT const&, FSM&, SourceState&, TargetState&)
            {
                LOG(STATE) << "Transition from " << typeid(SourceState).name() << " to " << typeid(TargetState).name() << " with event:" << typeid(EVT).name();
            }
        };
        struct InitFct
        {
            template <class EVT, class FSM, class SourceState, class TargetState>
            void operator()(EVT const&, FSM& fsm, SourceState&, TargetState&)
            {
                fsm.fState = INITIALIZING;
                fsm.Init();
            }
        };
        struct SetOutputFct
        {
            template <class EVT, class FSM, class SourceState, class TargetState>
            void operator()(EVT const&, FSM& fsm, SourceState&, TargetState&)
            {
                fsm.fState = SETTINGOUTPUT;
                fsm.InitOutput();
            }
        };
        struct SetInputFct
        {
            template <class EVT, class FSM, class SourceState, class TargetState>
            void operator()(EVT const&, FSM& fsm, SourceState&, TargetState&)
            {
                fsm.fState = SETTINGINPUT;
                fsm.InitInput();
            }
        };
        struct RunFct
        {
            template <class EVT, class FSM, class SourceState, class TargetState>
            void operator()(EVT const&, FSM& fsm, SourceState&, TargetState&)
            {
                fsm.fState = RUNNING;
                fsm.running_state = boost::thread(boost::bind(&FairMQFSM_::Run, &fsm));
            }
        };
        struct StopFct
        {
            template <class EVT, class FSM, class SourceState, class TargetState>
            void operator()(EVT const&, FSM& fsm, SourceState&, TargetState&)
            {
                fsm.fState = IDLE;
                fsm.running_state.join();
                fsm.Shutdown();
            }
        };
        struct PauseFct
        {
            template <class EVT, class FSM, class SourceState, class TargetState>
            void operator()(EVT const&, FSM& fsm, SourceState&, TargetState&)
            {
                fsm.fState = WAITING;
                fsm.running_state.join();
                fsm.Pause();
            }
        };
        // actions to be overwritten by derived classes
        virtual void Init()
        {
        }
        virtual void Run()
        {
        }
        virtual void Pause()
        {
        }
        virtual void Shutdown()
        {
        }
        virtual void InitOutput()
        {
        }
        virtual void InitInput()
        {
        }
        // Transition table for FairMQFMS
        struct transition_table : mpl::vector<
            //    Start     Event     Next    Action    Guard
            //  +---------+---------+-------+---------+--------+
            msmf::Row<IDLE_FSM, INIT, INITIALIZING_FSM, InitFct, msmf::none>,
            msmf::Row<IDLE_FSM, END, msmf::none, TestFct, msmf::none>, // this is an invalid transition...
            msmf::Row<INITIALIZING_FSM, SETOUTPUT, SETTINGOUTPUT_FSM, SetOutputFct, msmf::none>,
            msmf::Row<SETTINGOUTPUT_FSM, SETINPUT, SETTINGINPUT_FSM, SetInputFct, msmf::none>,
            msmf::Row<SETTINGINPUT_FSM, PAUSE, WAITING_FSM, PauseFct, msmf::none>,
            msmf::Row<SETTINGINPUT_FSM, RUN, RUNNING_FSM, RunFct, msmf::none>,
            msmf::Row<WAITING_FSM, RUN, RUNNING_FSM, RunFct, msmf::none>,
            msmf::Row<WAITING_FSM, STOP, IDLE_FSM, StopFct, msmf::none>,
            msmf::Row<RUNNING_FSM, PAUSE, WAITING_FSM, PauseFct, msmf::none>,
            msmf::Row<RUNNING_FSM, STOP, IDLE_FSM, StopFct, msmf::none> >
        {
        };
        // Replaces the default no-transition response.
        template <class FSM, class Event>
        void no_transition(Event const& e, FSM&, int state)
        {
            LOG(STATE) << "no transition from state " << state << " on event " << typeid(e).name() << std::endl;
        }
        // this is to run certain functions (e.g. Run()) as separate task
        boost::thread running_state;
        // backward compatibility to FairMQStateMachine
        enum State
        {
            IDLE,
            INITIALIZING,
            SETTINGOUTPUT,
            SETTINGINPUT,
            WAITING,
            RUNNING
        };
        State fState;
    };
    typedef msm::back::state_machine<FairMQFSM_> FairMQFSM;
}

class FairMQStateMachine : public FairMQFSM::FairMQFSM
{
  public:
    enum Event
    {
        INIT,
        SETOUTPUT,
        SETINPUT,
        PAUSE,
        RUN,
        STOP,
        END
    };
    FairMQStateMachine();
    virtual ~FairMQStateMachine();
    void ChangeState(int event);
};

#endif /* FAIRMQSTATEMACHINE_H_ */
