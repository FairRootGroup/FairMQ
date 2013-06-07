/*
 * FairMQSink.cxx
 *
 *  Created on: Jan 9, 2013
 *      Author: dklein
 */

#include "FairMQSink.h"
#include "FairMQLogger.h"

FairMQSink::FairMQSink()
{
}

void FairMQSink::Run()
{
  void* status; //necessary for pthread_join
  FairMQLogger::GetInstance()->Log(FairMQLogger::INFO, ">>>>>>> Run <<<<<<<");

  pthread_t logger;
  pthread_create(&logger, NULL, &FairMQDevice::callLogSocketRates, this);

  // Initialize poll set
  zmq_pollitem_t items[] = {
    { *(fPayloadInputs->at(0)->GetSocket()), 0, ZMQ_POLLIN, 0 }
  };

  while (true) {
    FairMQMessage msg;

    zmq_poll(items, 1, -1);

    if (items[0].revents & ZMQ_POLLIN) {
      fPayloadInputs->at(0)->Receive(&msg);
    }
  }

  pthread_join(logger, &status);
}

FairMQSink::~FairMQSink()
{
}

