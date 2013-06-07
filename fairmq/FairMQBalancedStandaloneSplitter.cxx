/*
 * FairMQBalancedStandaloneSplitter.cxx
 *
 *  Created on: Dec 6, 2012
 *      Author: dklein
 */

#include "FairMQBalancedStandaloneSplitter.h"

#include "FairMQLogger.h"

FairMQBalancedStandaloneSplitter::FairMQBalancedStandaloneSplitter()
{
}

FairMQBalancedStandaloneSplitter::~FairMQBalancedStandaloneSplitter()
{
}

void FairMQBalancedStandaloneSplitter::Run()
{
  void* status; //necessary for pthread_join
  FairMQLogger::GetInstance()->Log(FairMQLogger::INFO, ">>>>>>> Run <<<<<<<");

  pthread_t logger;
  pthread_create(&logger, NULL, &FairMQDevice::callLogSocketRates, this);

  // Initialize poll set
  zmq_pollitem_t items[] = {
    { *(fPayloadInputs->at(0)->GetSocket()), 0, ZMQ_POLLIN, 0 }
  };

  Bool_t received = false;
  Bool_t direction = false;
  while (true) {
    FairMQMessage msg;

    zmq_poll(items, 1, -1);

    if (items[0].revents & ZMQ_POLLIN) {
      received = fPayloadInputs->at(0)->Receive(&msg);
    }

    if (received) {
      if (direction) {
        fPayloadOutputs->at(0)->Send(&msg);
      } else {
        fPayloadOutputs->at(1)->Send(&msg);
      }
      direction = !direction;
    }
  }

  pthread_join(logger, &status);
}


