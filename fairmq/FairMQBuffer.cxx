/*
 * FairMQBuffer.cxx
 *
 *  Created on: Oct 25, 2012
 *      Author: dklein
 */

#include <iostream>

#include <boost/thread.hpp>
#include <boost/bind.hpp>

#include "FairMQBuffer.h"
#include "FairMQLogger.h"

FairMQBuffer::FairMQBuffer()
{
}

void FairMQBuffer::Run()
{
  FairMQLogger::GetInstance()->Log(FairMQLogger::INFO, ">>>>>>> Run <<<<<<<");

  boost::thread rateLogger(boost::bind(&FairMQDevice::LogSocketRates, this));

  // Initialize poll set
  zmq_pollitem_t items[] = {
    { *(fPayloadInputs->at(0)->GetSocket()), 0, ZMQ_POLLIN, 0 }
  };

  Bool_t received = false;
  while ( fState == RUNNING ) {
    FairMQMessage msg;

    zmq_poll(items, 1, 100);

    if (items[0].revents & ZMQ_POLLIN) {
      received = fPayloadInputs->at(0)->Receive(&msg);
    }

    if (received) {
      fPayloadOutputs->at(0)->Send(&msg);
      received = false;
    }
  }

  rateLogger.interrupt();
  rateLogger.join();
}

FairMQBuffer::~FairMQBuffer()
{
}

