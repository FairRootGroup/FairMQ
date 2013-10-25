/*
 * FairMQSamplerTask.cxx
 *
 *  Created on: Nov 22, 2012
 *      Author: dklein
 */

#include "FairMQSamplerTask.h"


FairMQSamplerTask::FairMQSamplerTask(const Text_t* name, Int_t iVerbose) :
  FairTask(name, iVerbose),
  fInput(NULL),
  fBranch(""),
  fOutput(new FairMQMessage)
{
}

FairMQSamplerTask::FairMQSamplerTask() :
  FairTask( "Abstract base task used for loading a branch from a root file into memory"),
  fInput(NULL),
  fBranch(""),
  fOutput(new FairMQMessage)
{
}

FairMQSamplerTask::~FairMQSamplerTask()
{
  delete fInput;
  //delete fOutput; // leave fOutput in memory, because it is needed even after FairMQSamplerTask is terminated. ClearOutput will clean it when it is no longer needed.
}

InitStatus FairMQSamplerTask::Init()
{
  FairRootManager* ioman = FairRootManager::Instance();
  fInput = (TClonesArray*) ioman->GetObject(fBranch.Data());

  return kSUCCESS;
}

void FairMQSamplerTask::SetBranch(TString branch)
{
  fBranch = branch;
}

FairMQMessage* FairMQSamplerTask::GetOutput()
{
  return fOutput;
}

void FairMQSamplerTask::ClearOutput(void* data, void* hint)
{
  free (data);
}

