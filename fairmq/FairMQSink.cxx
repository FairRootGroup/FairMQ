/*
 * FairMQSink.cxx
 *
 *  Created on: Jan 9, 2013
 *      Author: dklein
 */

#include <iostream>

#include <boost/thread.hpp>
#include <boost/bind.hpp>

#include "FairMQSink.h"
#include "FairMQLogger.h"

FairMQSink::FairMQSink()
{
}

void FairMQSink::Run()
{
  void* status; //necessary for pthread_join
  FairMQLogger::GetInstance()->Log(FairMQLogger::INFO, ">>>>>>> Run <<<<<<<");

  boost::thread rateLogger(boost::bind(&FairMQDevice::LogSocketRates, this));

  // Initialize poll set
  zmq_pollitem_t items[] = {
    { *(fPayloadInputs->at(0)->GetSocket()), 0, ZMQ_POLLIN, 0 }
  };

  while ( fState == RUNNING ) {
    FairMQMessage msg;

    zmq_poll(items, 1, 100);

    if (items[0].revents & ZMQ_POLLIN) {
      fPayloadInputs->at(0)->Receive(&msg);
    }
  }

  rateLogger.interrupt();
  rateLogger.join();
}

FairMQSink::~FairMQSink()
{
}

