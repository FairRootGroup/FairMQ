/**
 * FairMQSampler.cpp
 *
 * @since 2012-09-27
 * @author D. Klein, A. Rybalchenko
 */

#include <vector>
#include <iostream>

#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/timer/timer.hpp>

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
  if(fFairRunAna) {
    fFairRunAna->TerminateRun();
  }
}

void FairMQSampler::Init()
{
  FairMQDevice::Init();

  fSamplerTask->SetBranch(fBranch);

  fFairRunAna->SetInputFile(TString(fInputFile));
  TString output=fInputFile;
  output.Append(".out.root");
  fFairRunAna->SetOutputFile(output.Data());

  fFairRunAna->AddTask(fSamplerTask);

  FairRuntimeDb* rtdb = fFairRunAna->GetRuntimeDb();
  FairParRootFileIo* parInput1 = new FairParRootFileIo();
  parInput1->open(TString(fParFile).Data());
  rtdb->setFirstInput(parInput1);
  rtdb->print();

  fFairRunAna->Init();
  //fFairRunAna->Run(0, 0);
  FairRootManager* ioman = FairRootManager::Instance();
  fNumEvents = Int_t((ioman->GetInChain())->GetEntries());
}

void FairMQSampler::Run()
{
  FairMQLogger::GetInstance()->Log(FairMQLogger::INFO, ">>>>>>> Run <<<<<<<");
  boost::this_thread::sleep(boost::posix_time::milliseconds(1000));

  boost::thread rateLogger(boost::bind(&FairMQDevice::LogSocketRates, this));
  boost::thread resetEventCounter(boost::bind(&FairMQSampler::ResetEventCounter, this));
  //boost::thread commandListener(boost::bind(&FairMQSampler::ListenToCommands, this));

  int sentMsgs = 0;

  boost::timer::auto_cpu_timer timer;

  std::cout << "Number of events to process: " << fNumEvents << std::endl;

  Long64_t eventNr = 0;

//  while ( fState == RUNNING ) {

  for ( /* eventNr */ ; eventNr < fNumEvents; eventNr++ ) {
    fFairRunAna->RunMQ(eventNr);

    fPayloadOutputs->at(0)->Send(fSamplerTask->GetOutput());
    sentMsgs++;

    --fEventCounter;

    while (fEventCounter == 0) {
      boost::this_thread::sleep(boost::posix_time::milliseconds(1));
    }

    if( fState != RUNNING ) { break; }
  }

  boost::this_thread::interruption_point();
//  }

  boost::timer::cpu_times const elapsed_time(timer.elapsed());

  std::cout << "Sent everything in:\n" << boost::timer::format(elapsed_time, 2) << std::endl;
  std::cout << "Sent " << sentMsgs << " messages!" << std::endl;

  //boost::this_thread::sleep(boost::posix_time::milliseconds(5000));

  rateLogger.interrupt();
  rateLogger.join();
  resetEventCounter.interrupt();
  resetEventCounter.join();
  //commandListener.interrupt();
  //commandListener.join();
}

void FairMQSampler::ResetEventCounter()
{
  while ( true ) {
    try {
      fEventCounter = fEventRate / 100;
      boost::this_thread::sleep(boost::posix_time::milliseconds(10));
    } catch (boost::thread_interrupted&) {
      std::cout << "resetEventCounter interrupted" << std::endl;
      break;
    }
  }
  FairMQLogger::GetInstance()->Log(FairMQLogger::DEBUG, ">>>>>>> stopping resetEventCounter <<<<<<<");
}

void FairMQSampler::ListenToCommands()
{
  FairMQLogger::GetInstance()->Log(FairMQLogger::INFO, ">>>>>>> ListenToCommands <<<<<<<");

  bool received = false;

  while ( true ) {
    try {
      FairMQMessage msg;

      received = fPayloadInputs->at(0)->Receive(&msg);

      if (received) {
        //command handling goes here.
        FairMQLogger::GetInstance()->Log(FairMQLogger::INFO, "> received command <");
        received = false;
      }

      boost::this_thread::interruption_point();
    } catch (boost::thread_interrupted&) {
      std::cout << "commandListener interrupted" << std::endl;
      break;
    }
  }
  FairMQLogger::GetInstance()->Log(FairMQLogger::DEBUG, ">>>>>>> stopping commandListener <<<<<<<");
}

void FairMQSampler::SetProperty(const Int_t& key, const TString& value, const Int_t& slot/*= 0*/)
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

TString FairMQSampler::GetProperty(const Int_t& key, const TString& default_/*= ""*/, const Int_t& slot/*= 0*/)
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

void FairMQSampler::SetProperty(const Int_t& key, const Int_t& value, const Int_t& slot/*= 0*/)
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

Int_t FairMQSampler::GetProperty(const Int_t& key, const Int_t& default_/*= 0*/, const Int_t& slot/*= 0*/)
{
  switch (key) {
  case EventRate:
    return fEventRate;
  default:
    return FairMQDevice::GetProperty(key, default_, slot);
  }
}

