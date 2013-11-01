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

  zmq_pollitem_t items[fNumInputs];
  for (Int_t iInput = 0; iInput < fNumInputs; iInput++) {
    zmq_pollitem_t tempitem( {*(fPayloadInputs->at(iInput)->GetSocket()), 0, ZMQ_POLLIN, 0});
    items[iInput] =  tempitem;
  }

  Bool_t received = false;

  while ( fState == RUNNING ) {
    FairMQMessage msg;

    zmq_poll(items, fNumInputs, 100);

    for(Int_t iItem = 0; iItem < fNumInputs; iItem++) {
      if (items[iItem].revents & ZMQ_POLLIN) {
        received = fPayloadInputs->at(iItem)->Receive(&msg);
      }
      if (received) {
        fPayloadOutputs->at(0)->Send(&msg);
        received = false;
      }
    }
  }

  rateLogger.interrupt();
  rateLogger.join();
}

