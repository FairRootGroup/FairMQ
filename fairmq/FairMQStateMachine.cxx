/**
 * FairMQStateMachine.cxx
 *
 * @since 2012-10-25
 * @author D. Klein, A. Rybalchenko
 */

#include <boost/bind.hpp>
#include <boost/thread.hpp>

#include "FairMQStateMachine.h"
#include "FairMQLogger.h"


FairMQStateMachine::FairMQStateMachine() :
  fState(IDLE)
{
}

void FairMQStateMachine::ChangeState(int event)
{
  switch(fState) {

  case IDLE:
    switch(event) {

    case INIT:
      LOG(STATE) << "IDLE --init--> INITIALIZING";
      fState = INITIALIZING;
      Init();
      return;

    case END:
      LOG(STATE) << "IDLE --end--> (o)";
      return;

    default:
      return;
    }
    break;


  case INITIALIZING:
    switch(event) {

    case SETOUTPUT:
      LOG(STATE) << "INITIALIZING --bind--> SETTINGOUTPUT";
      fState = SETTINGOUTPUT;
      InitOutput();
      return;

    default:
      return;
    }
    break;


  case SETTINGOUTPUT:
    switch(event) {

    case SETINPUT:
      LOG(STATE) << "SETTINGOUTPUT --connect--> SETTINGINPUT";
      fState = SETTINGINPUT;
      InitInput();
      return;

    default:
      return;
    }
    break;


  case SETTINGINPUT:
    switch(event) {

    case PAUSE:
      LOG(STATE) << "SETTINGINPUT --pause--> WAITING";
      fState = WAITING;
      Pause();
      return;

    case RUN:
      LOG(STATE) << "SETTINGINPUT --run--> RUNNING";
      fState = RUNNING;
      running_state = boost::thread(boost::bind(&FairMQStateMachine::Run, this));
      return;

    default:
      return;
    }
    break;


  case WAITING:
    switch(event) {

    case RUN:
      LOG(STATE) << "WAITING --run--> RUNNING";
      fState = RUNNING;
      running_state = boost::thread(boost::bind(&FairMQStateMachine::Run, this));
      return;

    case STOP:
      LOG(STATE) << "WAITING --stop--> IDLE";
      fState = IDLE;
      Shutdown();
      return;

    default:
      return;
    }
    break;


  case RUNNING:
    switch(event) {

    case PAUSE:
      LOG(STATE) << "RUNNING --pause--> WAITING";
      fState = WAITING;
      running_state.join();
      return;

    case STOP:
      LOG(STATE) << "RUNNING --stop--> IDLE";
      fState = IDLE;
      running_state.join();
      Shutdown();
      return;

    default:
      return;
    }
    break;


  default:
    break;

  }//switch fState
}

void FairMQStateMachine::Init()
{
}

void FairMQStateMachine::Run()
{
}

void FairMQStateMachine::Pause()
{
}

void FairMQStateMachine::Shutdown()
{
}

void FairMQStateMachine::InitOutput()
{
}

void FairMQStateMachine::InitInput()
{
}

FairMQStateMachine::~FairMQStateMachine()
{
}

