/*
 * FairMQStandaloneMerger.cxx
 *
 *  Created on: Dec 6, 2012
 *      Author: dklein
 */

#include <boost/thread.hpp>
#include <boost/bind.hpp>

#include "FairMQLogger.h"
#include "FairMQStandaloneMerger.h"

FairMQStandaloneMerger::FairMQStandaloneMerger()
{
}

FairMQStandaloneMerger::~FairMQStandaloneMerger()
{
}

void FairMQStandaloneMerger::Run()
{
  FairMQLogger::GetInstance()->Log(FairMQLogger::INFO, ">>>>>>> Run <<<<<<<");

  boost::thread rateLogger(boost::bind(&FairMQDevice::LogSocketRates, this));

  // Initialize poll set
  zmq_pollitem_t items[] = {
    { *(fPayloadInputs->at(0)->GetSocket()), 0, ZMQ_POLLIN, 0 },
    { *(fPayloadInputs->at(1)->GetSocket()), 0, ZMQ_POLLIN, 0 }
  };

  Bool_t received = false;

  while ( fState == RUNNING ) {
    FairMQMessage msg;

    zmq_poll(items, fNumInputs, 100);

    if (items[0].revents & ZMQ_POLLIN) {
      received = fPayloadInputs->at(0)->Receive(&msg);
    }

    if (received) {
      fPayloadOutputs->at(0)->Send(&msg);
      received = false;
    }

    if (items[1].revents & ZMQ_POLLIN) {
      received = fPayloadInputs->at(1)->Receive(&msg);
    }

    if (received) {
      fPayloadOutputs->at(0)->Send(&msg);
      received = false;
    }

  }

  rateLogger.interrupt();
  rateLogger.join();
}

