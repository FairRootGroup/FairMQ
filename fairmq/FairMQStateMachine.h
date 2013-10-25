/*
 * FairMQStateMachine.h
 *
 *  Created on: Oct 25, 2012
 *      Author: dklein
 */



#ifndef FAIRMQSTATEMACHINE_H_
#define FAIRMQSTATEMACHINE_H_

#include <boost/thread.hpp>


class FairMQStateMachine
{
  public:
    enum State {
      IDLE, INITIALIZING, SETTINGOUTPUT, SETTINGINPUT, WAITING, RUNNING
    };
    enum Event {
      INIT, SETOUTPUT, SETINPUT, PAUSE, RUN, STOP, END
    };
    FairMQStateMachine();
    void ChangeState(int event);
    virtual ~FairMQStateMachine();

  protected:
    State fState;
    Event fEvent;
    virtual void Init();
    virtual void Run();
    virtual void Pause();
    virtual void Shutdown();
    virtual void InitOutput();
    virtual void InitInput();
    boost::thread running_state;
};

#endif /* FAIRMQSTATEMACHINE_H_ */
