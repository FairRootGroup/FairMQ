/*
 * FairMQStateMachine.cxx
 *
 *  Created on: Oct 25, 2012
 *      Author: dklein
 */

#include "FairMQStateMachine.h"


FairMQStateMachine::FairMQStateMachine() :
  fState(START)
{
}

FairMQStateMachine::RunStateMachine()
{
  void* status; //necessary for pthread_join
  pthread_t state;

  changeState(INIT);

  while(fState != END) {
    switch(fState) {
    case INIT:
      pthread_create(&state, NULL, &FairMQStateMachine::Init, this);
      break;

    }
    pthread_join(state, &status);
  }



}



FairMQStateMachine::~FairMQStateMachine()
{
}

