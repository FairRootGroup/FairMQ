/*
 * FairMQProcessor.cxx
 *
 *  Created on: Dec 6, 2012
 *      Author: dklein
 */

#include "FairMQProcessor.h"
#include "FairMQLogger.h"

FairMQProcessor::FairMQProcessor() :
  fTask(NULL)
{
}

FairMQProcessor::~FairMQProcessor()
{
  delete fTask;
}

void FairMQProcessor::SetTask(FairMQProcessorTask* task)
{
  fTask = task;
}

void FairMQProcessor::Init()
{
  FairMQDevice::Init();

  fTask->InitTask();
}

void FairMQProcessor::Run()
{
  FairMQLogger::GetInstance()->Log(FairMQLogger::INFO, ">>>>>>> Run <<<<<<<");

  void* status; //necessary for pthread_join
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
      fTask->Exec(&msg, NULL);

      fPayloadOutputs->at(0)->Send(&msg);
    }
  }

  pthread_join(logger, &status);
}

