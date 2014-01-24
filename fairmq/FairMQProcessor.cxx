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
  fProcessorTask(NULL)
{
}

FairMQProcessor::~FairMQProcessor()
{
  delete fProcessorTask;
}

void FairMQProcessor::SetTask(FairMQProcessorTask* task)
{
  fProcessorTask = task;
}

void FairMQProcessor::Init()
{
  FairMQDevice::Init();

  fProcessorTask->InitTask();
}

void FairMQProcessor::Run()
{
  FairMQLogger::GetInstance()->Log(FairMQLogger::INFO, ">>>>>>> Run <<<<<<<");

  boost::thread rateLogger(boost::bind(&FairMQDevice::LogSocketRates, this));

  int receivedMsgs = 0;
  int sentMsgs = 0;

  bool received = false;

  while ( fState == RUNNING ) {
    FairMQMessage* msg = fTransportFactory->CreateMessage();

    received = fPayloadInputs->at(0)->Receive(msg);
    receivedMsgs++;

    if (received) {
      fProcessorTask->Exec(msg, NULL);

      fPayloadOutputs->at(0)->Send(msg);
      sentMsgs++;
      received = false;
    }

    delete msg;
  }

  cout << "I've received " << receivedMsgs << " and sent " << sentMsgs << " messages!" << endl;

  boost::this_thread::sleep(boost::posix_time::milliseconds(5000));

  rateLogger.interrupt();
  rateLogger.join();
}

