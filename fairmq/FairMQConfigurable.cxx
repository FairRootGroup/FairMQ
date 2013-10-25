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

void FairMQConfigurable::SetProperty(const Int_t& key, const TString& value, const Int_t& slot/*= 0*/)
{
}

TString FairMQConfigurable::GetProperty(const Int_t& key, const TString& default_/*= ""*/, const Int_t& slot/*= 0*/)
{
  return default_;
}

void FairMQConfigurable::SetProperty(const Int_t& key, const Int_t& value, const Int_t& slot/*= 0*/)
{
}

Int_t FairMQConfigurable::GetProperty(const Int_t& key, const Int_t& default_/*= 0*/, const Int_t& slot/*= 0*/)
{
  return default_;
}

FairMQConfigurable::~FairMQConfigurable()
{
}


