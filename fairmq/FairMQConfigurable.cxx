/*
 * FairMQConfigurable.cxx
 *
 *  Created on: Oct 25, 2012
 *      Author: dklein
 */

#include "FairMQConfigurable.h"


FairMQConfigurable::FairMQConfigurable()
{
}

void FairMQConfigurable::SetProperty(Int_t key, TString value, Int_t slot/*= 0*/)
{
}

TString FairMQConfigurable::GetProperty(Int_t key, TString default_/*= ""*/, Int_t slot/*= 0*/)
{
  return default_;
}

void FairMQConfigurable::SetProperty(Int_t key, Int_t value, Int_t slot/*= 0*/)
{
}

Int_t FairMQConfigurable::GetProperty(Int_t key, Int_t default_/*= 0*/, Int_t slot/*= 0*/)
{
  return default_;
}

FairMQConfigurable::~FairMQConfigurable()
{
}


