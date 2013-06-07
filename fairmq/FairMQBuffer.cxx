/*
 * FairMQBuffer.cxx
 *
 *  Created on: Oct 25, 2012
 *      Author: dklein
 */

#include "FairMQBuffer.h"
#include <iostream>
#include "FairMQLogger.h"


FairMQBuffer::FairMQBuffer()
{
}

void FairMQBuffer::Run()
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
  while (true) {
    FairMQMessage msg;

    zmq_poll(items, 1, -1);

    if (items[0].revents & ZMQ_POLLIN) {
      received = fPayloadInputs->at(0)->Receive(&msg);
    }

    if (received) {
      fPayloadOutputs->at(0)->Send(&msg);
    }
  }

  pthread_join(logger, &status);
}

FairMQBuffer::~FairMQBuffer()
{
}

