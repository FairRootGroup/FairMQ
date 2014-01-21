/**
 * FairMQMerger.cxx
 *
 * @since 2012-12-06
 * @author D. Klein, A. Rybalchenko
 */

#include <boost/thread.hpp>
#include <boost/bind.hpp>

#include "FairMQLogger.h"
#include "FairMQMerger.h"


FairMQMerger::FairMQMerger()
{
}

FairMQMerger::~FairMQMerger()
{
}

void FairMQMerger::Run()
{
  FairMQLogger::GetInstance()->Log(FairMQLogger::INFO, ">>>>>>> Run <<<<<<<");

  boost::thread rateLogger(boost::bind(&FairMQDevice::LogSocketRates, this));

  zmq_pollitem_t items[fNumInputs];

  for (int i = 0; i < fNumInputs; i++) {
    items[i].socket = fPayloadInputs->at(i)->GetSocket();
    items[i].fd = 0;
    items[i].events = ZMQ_POLLIN;
    items[i].revents = 0;
  }

  bool received = false;

  while ( fState == RUNNING ) {
    FairMQMessage* msg = fTransportFactory->CreateMessage();

    zmq_poll(items, fNumInputs, 100);

    for(int i = 0; i < fNumInputs; i++) {
      if (items[i].revents & ZMQ_POLLIN) {
        received = fPayloadInputs->at(i)->Receive(msg);
      }
      if (received) {
        fPayloadOutputs->at(0)->Send(msg);
        received = false;
      }
    }

    delete msg;
  }

  rateLogger.interrupt();
  rateLogger.join();
}

