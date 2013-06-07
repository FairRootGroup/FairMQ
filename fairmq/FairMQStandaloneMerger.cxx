/*
 * FairMQStandaloneMerger.cxx
 *
 *  Created on: Dec 6, 2012
 *      Author: dklein
 */

#include "FairMQStandaloneMerger.h"
#include "FairMQLogger.h"

FairMQStandaloneMerger::FairMQStandaloneMerger()
{
}

FairMQStandaloneMerger::~FairMQStandaloneMerger()
{
}

void FairMQStandaloneMerger::Run()
{
  FairMQLogger::GetInstance()->Log(FairMQLogger::INFO, ">>>>>>> Run <<<<<<<");

  Bool_t received0 = false;
  Bool_t received1 = false;
  FairMQMessage* msg0 = NULL;
  FairMQMessage* msg1 = NULL;
  TString source0 = fPayloadInputs->at(0)->GetId();
  TString source1 = fPayloadInputs->at(1)->GetId();
  Int_t size0 = 0;
  Int_t size1 = 0;

  // Initialize poll set
  zmq_pollitem_t items[] = {
    { *(fPayloadInputs->at(0)->GetSocket()), 0, ZMQ_POLLIN, 0 },
    { *(fPayloadInputs->at(1)->GetSocket()), 0, ZMQ_POLLIN, 0 }
  };

  void* status; //necessary for pthread_join
  pthread_t logger;
  pthread_create(&logger, NULL, &FairMQDevice::callLogSocketRates, this);

  while (true) {
    msg0 = new FairMQMessage();
    msg1 = new FairMQMessage();

    zmq_poll(items, 2, -1);

    if (items[0].revents & ZMQ_POLLIN) {
      received0 = fPayloadInputs->at(0)->Receive(msg0);
    }

    if (items[1].revents & ZMQ_POLLIN) {
      received1 = fPayloadInputs->at(1)->Receive(msg1);
    }

    if (received0) {
      size0 = msg0->Size();
      fPayloadOutputs->at(0)->Send(msg0);

      received0 = false;
    }

    if (received1) {
      size1 = msg1->Size();
      fPayloadOutputs->at(0)->Send(msg1);

      received1 = false;
    }

    delete msg0;
    delete msg1;
  }

  pthread_join(logger, &status);
}

