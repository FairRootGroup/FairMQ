/**
 * FairMQSplitter.cxx
 *
 * @since 2012-12-06
 * @author D. Klein, A. Rybalchenko
 */

#include <boost/thread.hpp>
#include <boost/bind.hpp>

#include "FairMQLogger.h"
#include "FairMQSplitter.h"


FairMQSplitter::FairMQSplitter()
{
}

FairMQSplitter::~FairMQSplitter()
{
}

void FairMQSplitter::Run()
{
  LOG(INFO) << ">>>>>>> Run <<<<<<<";

  boost::thread rateLogger(boost::bind(&FairMQDevice::LogSocketRates, this));

  bool received = false;
  int direction = 0;

  while ( fState == RUNNING ) {
    FairMQMessage* msg = fTransportFactory->CreateMessage();

    received = fPayloadInputs->at(0)->Receive(msg);

    if (received) {
      fPayloadOutputs->at(direction)->Send(msg);
      direction++;
      if (direction >= fNumOutputs) {
        direction = 0;
      }
      received = false;
    }

    delete msg;
  }

  rateLogger.interrupt();
  rateLogger.join();
}
