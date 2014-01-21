/**
 * FairMQConfigurable.cxx
 *
 * @since 2012-10-25
 * @author D. Klein, A. Rybalchenko
 */

#include "FairMQConfigurable.h"


FairMQConfigurable::FairMQConfigurable()
{
}

void FairMQConfigurable::SetProperty(const int& key, const std::string& value, const int& slot/*= 0*/)
{
}

std::string FairMQConfigurable::GetProperty(const int& key, const std::string& default_/*= ""*/, const int& slot/*= 0*/)
{
  return default_;
}

void FairMQConfigurable::SetProperty(const int& key, const int& value, const int& slot/*= 0*/)
{
}

int FairMQConfigurable::GetProperty(const int& key, const int& default_/*= 0*/, const int& slot/*= 0*/)
{
  return default_;
}

FairMQConfigurable::~FairMQConfigurable()
{
}


