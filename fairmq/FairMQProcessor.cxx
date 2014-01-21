/**
 * FairMQProcessor.cxx
 *
 * @since 2012-12-06
 * @author D. Klein, A. Rybalchenko
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

  int receivedMsgs = 0;
  int sentMsgs = 0;

  bool received = false;

  while ( fState == RUNNING ) {
    FairMQMessage* msg = new FairMQMessageZMQ();

    received = fPayloadInputs->at(0)->Receive(msg);
    receivedMsgs++;

    if (received) {
      fTask->Exec(msg, NULL);

      fPayloadOutputs->at(0)->Send(msg);
      sentMsgs++;
      received = false;
    }

    delete msg;
  }

  std::cout << "I've received " << receivedMsgs << " and sent " << sentMsgs << " messages!" << std::endl;

  boost::this_thread::sleep(boost::posix_time::milliseconds(5000));

  rateLogger.interrupt();
  rateLogger.join();
}

