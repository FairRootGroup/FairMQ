/**
 * FairMQBinSampler.cpp
 *
 * @since 2013-04-23
 * @author D. Klein, A. Rybalchenko
 */

#include <vector>
#include <stdlib.h>     /* srand, rand */
#include <time.h>       /* time */

#include <boost/thread.hpp>
#include <boost/bind.hpp>

#include "FairMQBinSampler.h"
#include "FairMQLogger.h"


FairMQBinSampler::FairMQBinSampler() :
  fEventSize(10000),
  fEventRate(1),
  fEventCounter(0)
{
}

FairMQBinSampler::~FairMQBinSampler()
{
}

void FairMQBinSampler::Init()
{
  FairMQDevice::Init();
}

void FairMQBinSampler::Run()
{
  LOG(INFO) << ">>>>>>> Run <<<<<<<";
  //boost::this_thread::sleep(boost::posix_time::milliseconds(1000));

  boost::thread rateLogger(boost::bind(&FairMQDevice::LogSocketRates, this));
  boost::thread resetEventCounter(boost::bind(&FairMQBinSampler::ResetEventCounter, this));

  srand(time(NULL));

  LOG(DEBUG) << "Message size: " << fEventSize * sizeof(Content) << " bytes.";

  while ( fState == RUNNING ) {

    Content* payload = new Content[fEventSize];

    for (int i = 0; i < fEventSize; ++i) {
      (&payload[i])->x = rand() % 100 + 1;
      (&payload[i])->y = rand() % 100 + 1;
      (&payload[i])->z = rand() % 100 + 1;
      (&payload[i])->a = (rand() % 100 + 1) / (rand() % 100 + 1);
      (&payload[i])->b = (rand() % 100 + 1) / (rand() % 100 + 1);
      // LOG(INFO) << (&payload[i])->x << " " << (&payload[i])->y << " " << (&payload[i])->z << " " << (&payload[i])->a << " " << (&payload[i])->b;
    }

    FairMQMessage* msg = fTransportFactory->CreateMessage(fEventSize * sizeof(Content));
    memcpy(msg->GetData(), payload, fEventSize * sizeof(Content));

    fPayloadOutputs->at(0)->Send(msg);

    --fEventCounter;

    while (fEventCounter == 0) {
      boost::this_thread::sleep(boost::posix_time::milliseconds(1));
    }

    delete[] payload;
    delete msg;
  }

  rateLogger.interrupt();
  resetEventCounter.interrupt();

  rateLogger.join();
  resetEventCounter.join();
}

void FairMQBinSampler::ResetEventCounter()
{
  while ( true ) {
    try {
      fEventCounter = fEventRate / 100;
      boost::this_thread::sleep(boost::posix_time::milliseconds(10));
    } catch (boost::thread_interrupted&) {
      break;
    }
  }
}

void FairMQBinSampler::Log(int intervalInMs)
{
  timestamp_t t0;
  timestamp_t t1;
  unsigned long bytes = fPayloadOutputs->at(0)->GetBytesTx();
  unsigned long messages = fPayloadOutputs->at(0)->GetMessagesTx();
  unsigned long bytesNew = 0;
  unsigned long messagesNew = 0;
  double megabytesPerSecond = 0;
  double messagesPerSecond = 0;

  t0 = get_timestamp();

  while (true) {
    boost::this_thread::sleep(boost::posix_time::milliseconds(intervalInMs));

    t1 = get_timestamp();

    bytesNew = fPayloadOutputs->at(0)->GetBytesTx();
    messagesNew = fPayloadOutputs->at(0)->GetMessagesTx();

    timestamp_t timeSinceLastLog_ms = (t1 - t0) / 1000.0L;

    megabytesPerSecond = ((double) (bytesNew - bytes) / (1024. * 1024.)) / (double) timeSinceLastLog_ms * 1000.;
    messagesPerSecond = (double) (messagesNew - messages) / (double) timeSinceLastLog_ms * 1000.;

    LOG(DEBUG) << "send " << messagesPerSecond << " msg/s, " << megabytesPerSecond << " MB/s";

    bytes = bytesNew;
    messages = messagesNew;
    t0 = t1;
  }
}

void FairMQBinSampler::SetProperty(const int key, const string& value, const int slot/*= 0*/)
{
  switch (key) {
  default:
    FairMQDevice::SetProperty(key, value, slot);
    break;
  }
}

string FairMQBinSampler::GetProperty(const int key, const string& default_/*= ""*/, const int slot/*= 0*/)
{
  switch (key) {
  default:
    return FairMQDevice::GetProperty(key, default_, slot);
  }
}

void FairMQBinSampler::SetProperty(const int key, const int value, const int slot/*= 0*/)
{
  switch (key) {
  case EventSize:
    fEventSize = value;
    break;
  case EventRate:
    fEventRate = value;
    break;
  default:
    FairMQDevice::SetProperty(key, value, slot);
    break;
  }
}

int FairMQBinSampler::GetProperty(const int key, const int default_/*= 0*/, const int slot/*= 0*/)
{
  switch (key) {
  case EventSize:
    return fEventSize;
  case EventRate:
    return fEventRate;
  default:
    return FairMQDevice::GetProperty(key, default_, slot);
  }
}
