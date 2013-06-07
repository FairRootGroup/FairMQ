/*
 * FairMQLogger.cxx
 *
 *  Created on: Dec 4, 2012
 *      Author: dklein
 */

#include "FairMQLogger.h"
#include <iostream>
#include <ctime>
#include <iomanip>


FairMQLogger* FairMQLogger::instance = NULL;

FairMQLogger* FairMQLogger::GetInstance()
{
  if (instance == NULL) {
    instance = new FairMQLogger();
  }
  return instance;
}

FairMQLogger* FairMQLogger::InitInstance(TString bindAddress)
{
  instance = new FairMQLogger(bindAddress);
  return instance;
}

FairMQLogger::FairMQLogger() :
  fBindAddress("")
{
}

FairMQLogger::FairMQLogger(TString bindAddress) :
  fBindAddress(bindAddress)
{
}

FairMQLogger::~FairMQLogger()
{
}

void FairMQLogger::Log(Int_t type, TString logmsg)
{
  timestamp_t tm = get_timestamp();
  timestamp_t ms = tm / 1000.0L;
  timestamp_t s = ms / 1000.0L;
  std::time_t t = s;
  std::size_t fractional_seconds = ms % 1000;
  Text_t mbstr[100];
  std::strftime(mbstr, 100, "%H:%M:%S:", std::localtime(&t));

  TString type_str;
  switch (type) {
  case DEBUG:
    type_str = "\033[01;34mDEBUG\033[0m";
    break;
  case INFO:
    type_str = "\033[01;32mINFO\033[0m";
    break;
  case ERROR:
    type_str = "\033[01;31mERROR\033[0m";
    break;
  default:
    break;
  }

  std::cout << "[\033[01;36m" <<  mbstr << fractional_seconds << "\033[0m]" << "[" << type_str << "]" << " " << logmsg << std::endl;
}

