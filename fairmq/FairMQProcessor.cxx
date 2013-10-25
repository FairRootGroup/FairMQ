/*
 * FairMQProcessor.cxx
 *
 *  Created on: Dec 6, 2012
 *      Author: dklein
 */

#include <boost/thread.hpp>
#include <boost/bind.hpp>

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

  boost::thread rateLogger(boost::bind(&FairMQDevice::LogSocketRates, this));

  // Initialize poll set
  zmq_pollitem_t items[] = {
    { *(fPayloadInputs->at(0)->GetSocket()), 0, ZMQ_POLLIN, 0 }
  };

  int receivedMsgs = 0;
  int sentMsgs = 0;

  Bool_t received = false;

  while ( fState == RUNNING ) {
    FairMQMessage msg;

    zmq_poll(items, 1, 100);

    if (items[0].revents & ZMQ_POLLIN) {
      received = fPayloadInputs->at(0)->Receive(&msg);
      receivedMsgs++;
    }

    if (received) {
      fTask->Exec(&msg, NULL);

      fPayloadOutputs->at(0)->Send(&msg);
      sentMsgs++;
      received = false;
    }
  }

  std::cout << "I've received " << receivedMsgs << " and sent " << sentMsgs << " messages!" << std::endl;

  boost::this_thread::sleep(boost::posix_time::milliseconds(5000));

  rateLogger.interrupt();
  rateLogger.join();
}

