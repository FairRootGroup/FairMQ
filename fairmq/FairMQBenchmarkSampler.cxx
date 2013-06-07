/*
 * FairMQBenchmarkSampler.cpp
 *
 *  Created on: Apr 23, 2013
 *      Author: dklein
 */
#include <vector>
#include <unistd.h>

#include "FairMQBenchmarkSampler.h"
#include "FairMQLogger.h"

FairMQBenchmarkSampler::FairMQBenchmarkSampler() :
  fEventSize(10000),
  fEventRate(1),
  fEventCounter(0)
{
}

FairMQBenchmarkSampler::~FairMQBenchmarkSampler()
{
}

void FairMQBenchmarkSampler::Init()
{
  FairMQDevice::Init();
}

void FairMQBenchmarkSampler::Run()
{
  void* status; //necessary for pthread_join
  FairMQLogger::GetInstance()->Log(FairMQLogger::INFO, ">>>>>>> Run <<<<<<<");
  usleep(1000000);

  pthread_t logger;
  pthread_create(&logger, NULL, &FairMQDevice::callLogSocketRates, this);

  pthread_t resetEventCounter;
  pthread_create(&resetEventCounter, NULL, &FairMQBenchmarkSampler::callResetEventCounter, this);

  void* buffer = operator new[](fEventSize);
  FairMQMessage* base_event = new FairMQMessage(buffer, fEventSize, NULL);

  while (true) {
    FairMQMessage event;
    event.Copy(base_event);

    fPayloadOutputs->at(0)->Send(&event);

    --fEventCounter;

    while (fEventCounter == 0) {
      usleep(1000);
    }
  }

  delete base_event;

  pthread_join(logger, &status);
  pthread_join(resetEventCounter, &status);
}

void* FairMQBenchmarkSampler::ResetEventCounter()
{
  while (true) {
    fEventCounter = fEventRate / 100;

    usleep(10000);
  }
}

void FairMQBenchmarkSampler::Log(Int_t intervalInMs)
{
  timestamp_t t0;
  timestamp_t t1;
  ULong_t bytes = fPayloadOutputs->at(0)->GetBytesTx();
  ULong_t messages = fPayloadOutputs->at(0)->GetMessagesTx();
  ULong_t bytesNew;
  ULong_t messagesNew;
  Double_t megabytesPerSecond = (bytesNew - bytes) / (1024 * 1024);
  Double_t messagesPerSecond = (messagesNew - messages);

  t0 = get_timestamp();

  while (true) {
    usleep(intervalInMs * 1000);

    t1 = get_timestamp();

    bytesNew = fPayloadOutputs->at(0)->GetBytesTx();
    messagesNew = fPayloadOutputs->at(0)->GetMessagesTx();

    timestamp_t timeSinceLastLog_ms = (t1 - t0) / 1000.0L;

    megabytesPerSecond = ((Double_t) (bytesNew - bytes) / (1024. * 1024.)) / (Double_t) timeSinceLastLog_ms * 1000.;
    messagesPerSecond = (Double_t) (messagesNew - messages) / (Double_t) timeSinceLastLog_ms * 1000.;

    std::stringstream logmsg;
    logmsg << "send " << messagesPerSecond << " msg/s, " << megabytesPerSecond << " MB/s";
    FairMQLogger::GetInstance()->Log(FairMQLogger::DEBUG, logmsg.str());

    bytes = bytesNew;
    messages = messagesNew;
    t0 = t1;
  }
}

void FairMQBenchmarkSampler::SetProperty(Int_t key, TString value, Int_t slot/*= 0*/)
{
  switch (key) {
  default:
    FairMQDevice::SetProperty(key, value, slot);
    break;
  }
}

TString FairMQBenchmarkSampler::GetProperty(Int_t key, TString default_/*= ""*/, Int_t slot/*= 0*/)
{
  switch (key) {
  default:
    return FairMQDevice::GetProperty(key, default_, slot);
  }
}

void FairMQBenchmarkSampler::SetProperty(Int_t key, Int_t value, Int_t slot/*= 0*/)
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

Int_t FairMQBenchmarkSampler::GetProperty(Int_t key, Int_t default_/*= 0*/, Int_t slot/*= 0*/)
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

