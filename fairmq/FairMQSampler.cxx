/*
 * FairMQSampler.cpp
 *
 *  Created on: Sep 27, 2012
 *      Author: dklein
 */
#include <vector>
#include <unistd.h>

#include "TList.h"
#include "TObjString.h"
#include "TClonesArray.h"
#include "FairParRootFileIo.h"
#include "FairRuntimeDb.h"
#include "TROOT.h"

#include "FairMQSampler.h"
#include "FairMQLogger.h"


FairMQSampler::FairMQSampler() :
  fFairRunAna(new FairRunAna()),
  fSamplerTask(NULL),
  fInputFile(""),
  fParFile(""),
  fBranch(""),
  fEventRate(1)
{
}

FairMQSampler::~FairMQSampler()
{
  delete fSamplerTask;
  delete fFairRunAna;
}

void FairMQSampler::Init()
{
  FairMQDevice::Init();

  fSamplerTask->SetBranch(fBranch);

  fFairRunAna->SetInputFile(TString(fInputFile));
  fFairRunAna->SetOutputFile("dummy.out");

  fFairRunAna->AddTask(fSamplerTask);

  FairRuntimeDb* rtdb = fFairRunAna->GetRuntimeDb();
  FairParRootFileIo* parInput1 = new FairParRootFileIo();
  parInput1->open(TString(fParFile).Data());
  rtdb->setFirstInput(parInput1);
  rtdb->print();

  // read complete file and extract digis.
  fFairRunAna->Init();
  fFairRunAna->Run(0, 0);
}

void FairMQSampler::Run()
{
  void* status; //necessary for pthread_join
  FairMQLogger::GetInstance()->Log(FairMQLogger::INFO, ">>>>>>> Run <<<<<<<");
  usleep(1000000);

  pthread_t logger;
  pthread_create(&logger, NULL, &FairMQDevice::callLogSocketRates, this);

  pthread_t resetEventCounter;
  pthread_create(&resetEventCounter, NULL, &FairMQSampler::callResetEventCounter, this);

  while (true) {
    for( std::vector<FairMQMessage*>::iterator itr = fSamplerTask->GetOutput()->begin(); itr != fSamplerTask->GetOutput()->end(); itr++ ) {
      FairMQMessage event;
      event.Copy(*itr);

      fPayloadOutputs->at(0)->Send(&event);

      --fEventCounter;

      while (fEventCounter == 0) {
        usleep(1000);
      }
    }

  }

  pthread_join(logger, &status);
  pthread_join(resetEventCounter, &status);
}

void* FairMQSampler::ResetEventCounter()
{
  while (true) {
    fEventCounter = fEventRate / 100;

    usleep(10000);
  }
}

void FairMQSampler::Log(Int_t intervalInMs)
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

void FairMQSampler::SetProperty(Int_t key, TString value, Int_t slot/*= 0*/)
{
  switch (key) {
  case InputFile:
    fInputFile = value;
    break;
  case ParFile:
    fParFile = value;
    break;
  case Branch:
    fBranch = value;
    break;
  default:
    FairMQDevice::SetProperty(key, value, slot);
    break;
  }
}

TString FairMQSampler::GetProperty(Int_t key, TString default_/*= ""*/, Int_t slot/*= 0*/)
{
  switch (key) {
  case InputFile:
    return fInputFile;
  case ParFile:
    return fParFile;
  case Branch:
    return fBranch;
  default:
    return FairMQDevice::GetProperty(key, default_, slot);
  }
}

void FairMQSampler::SetProperty(Int_t key, Int_t value, Int_t slot/*= 0*/)
{
  switch (key) {
  case EventRate:
    fEventRate = value;
    break;
  default:
    FairMQDevice::SetProperty(key, value, slot);
    break;
  }
}

Int_t FairMQSampler::GetProperty(Int_t key, Int_t default_/*= 0*/, Int_t slot/*= 0*/)
{
  switch (key) {
  case EventRate:
    return fEventRate;
  default:
    return FairMQDevice::GetProperty(key, default_, slot);
  }
}

