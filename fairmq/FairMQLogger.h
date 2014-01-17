/**
 * FairMQLogger.h
 *
 * @since 2012-12-04
 * @author D. Klein, A. Rybalchenko
 */

#ifndef FAIRMQLOGGER_H_
#define FAIRMQLOGGER_H_
#include <string>
#include <sstream>
#include <sys/time.h>
#include "Rtypes.h"
#include "TString.h"


class FairMQLogger
{
  private:
    static FairMQLogger* instance;
    TString fBindAddress;
  public:
    enum {
      DEBUG, INFO, ERROR, STATE
    };
    FairMQLogger();
    FairMQLogger(TString bindAdress);
    virtual ~FairMQLogger();
    void Log(Int_t type, TString logmsg);
    static FairMQLogger* GetInstance();
    static FairMQLogger* InitInstance(TString bindAddress);
};

typedef unsigned long long timestamp_t;

static timestamp_t get_timestamp ()
{
  struct timeval now;
  gettimeofday (&now, NULL);
  return now.tv_usec + (timestamp_t)now.tv_sec * 1000000;
}

#endif /* FAIRMQLOGGER_H_ */
