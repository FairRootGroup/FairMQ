/*
 * FairMQStateMachine.h
 *
 *  Created on: Oct 25, 2012
 *      Author: dklein
 */

#ifndef FAIRMQSTATEMACHINE_H_
#define FAIRMQSTATEMACHINE_H_


class FairMQStateMachine
{
  private:
    int fState;
  public:
    enum {
      START, INIT, BIND, CONNECT, RUN, PAUSE, SHUTDOWN, END
    };
    FairMQStateMachine();
    virtual void Init() = 0;
    virtual void Bind() = 0;
    virtual void Connect() = 0;
    virtual void Run() = 0;
    virtual void Pause() = 0;
    virtual void Shutdown() = 0;
    bool ChangeState(int new_state);
    void RunStateMachine();
    virtual ~FairMQStateMachine();
};

#endif /* FAIRMQSTATEMACHINE_H_ */
