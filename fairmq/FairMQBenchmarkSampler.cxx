/**
 * FairMQBenchmarkSampler.cpp
 *
 * @since 2013-04-23
 * @author D. Klein, A. Rybalchenko
 */

#include <vector>

#include <boost/thread.hpp>
#include <boost/bind.hpp>

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
  FairMQLogger::GetInstance()->Log(FairMQLogger::INFO, ">>>>>>> Run <<<<<<<");
  //boost::this_thread::sleep(boost::posix_time::milliseconds(1000));

  boost::thread rateLogger(boost::bind(&FairMQDevice::LogSocketRates, this));
  boost::thread resetEventCounter(boost::bind(&FairMQBenchmarkSampler::ResetEventCounter, this));

  void* buffer = operator new[](fEventSize);
  FairMQMessage* base_event = new FairMQMessage(buffer, fEventSize);

  while ( fState == RUNNING ) {
    FairMQMessage event;
    event.Copy(base_event);

    fPayloadOutputs->at(0)->Send(&event);

    --fEventCounter;

    while (fEventCounter == 0) {
      boost::this_thread::sleep(boost::posix_time::milliseconds(1));
    }
  }

  delete base_event;

  rateLogger.interrupt();
  resetEventCounter.interrupt();

  rateLogger.join();
  resetEventCounter.join();
}

void FairMQBenchmarkSampler::ResetEventCounter()
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
    boost::this_thread::sleep(boost::posix_time::milliseconds(intervalInMs));

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

void FairMQBenchmarkSampler::SetProperty(const Int_t& key, const TString& value, const Int_t& slot/*= 0*/)
{
  switch (key) {
  default:
    FairMQDevice::SetProperty(key, value, slot);
    break;
  }
}

TString FairMQBenchmarkSampler::GetProperty(const Int_t& key, const TString& default_/*= ""*/, const Int_t& slot/*= 0*/)
{
  switch (key) {
  default:
    return FairMQDevice::GetProperty(key, default_, slot);
  }
}

void FairMQBenchmarkSampler::SetProperty(const Int_t& key, const Int_t& value, const Int_t& slot/*= 0*/)
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

Int_t FairMQBenchmarkSampler::GetProperty(const Int_t& key, const Int_t& default_/*= 0*/, const Int_t& slot/*= 0*/)
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

