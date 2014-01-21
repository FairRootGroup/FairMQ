/**
 * FairMQLogger.cxx
 *
 * @since 2012-12-04
 * @author D. Klein, A. Rybalchenko
 */

#include <iostream>
#include <iomanip>
#include <ctime>

#include "FairMQLogger.h"


FairMQLogger* FairMQLogger::instance = NULL;

FairMQLogger* FairMQLogger::GetInstance()
{
  if (instance == NULL) {
    instance = new FairMQLogger();
  }
  return instance;
}

FairMQLogger* FairMQLogger::InitInstance(std::string bindAddress)
{
  instance = new FairMQLogger(bindAddress);
  return instance;
}

FairMQLogger::FairMQLogger() :
  fBindAddress("")
{
}

FairMQLogger::FairMQLogger(std::string bindAddress) :
  fBindAddress(bindAddress)
{
}

FairMQLogger::~FairMQLogger()
{
}

void FairMQLogger::Log(int type, std::string logmsg)
{
  timestamp_t tm = get_timestamp();
  timestamp_t ms = tm / 1000.0L;
  timestamp_t s = ms / 1000.0L;
  std::time_t t = s;
  std::size_t fractional_seconds = ms % 1000;
  char mbstr[100];
  std::strftime(mbstr, 100, "%H:%M:%S:", std::localtime(&t));

  std::string type_str;
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
    case STATE:
      type_str = "\033[01;33mSTATE\033[0m";
    default:
      break;
  }

  std::cout << "[\033[01;36m" <<  mbstr << fractional_seconds << "\033[0m]" << "[" << type_str << "]" << " " << logmsg << std::endl;
}

