/*
 * FairMQProxy.cxx
 *
 *  Created on: Oct 2, 2013
 *      Author: A. Rybalchenko
 */

#include <iostream>

#include <boost/thread.hpp>
#include <boost/bind.hpp>

#include "FairMQLogger.h"
#include "FairMQProxy.h"

FairMQProxy::FairMQProxy()
{
}

FairMQProxy::~FairMQProxy()
{
}

void FairMQProxy::Run()
{
  FairMQLogger::GetInstance()->Log(FairMQLogger::INFO, ">>>>>>> Run <<<<<<<");

  boost::thread rateLogger(boost::bind(&FairMQDevice::LogSocketRates, this));

  //TODO: check rateLogger output
  int rc = zmq_proxy(*(fPayloadInputs->at(0)->GetSocket()), *(fPayloadOutputs->at(0)->GetSocket()), NULL);
  if (rc == -1) {
    std::cout << "Error: proxy failed: " << strerror(errno) << std::endl;
  }

  //TODO: make proxy bind on both ends.

  rateLogger.interrupt();
  rateLogger.join();
}

