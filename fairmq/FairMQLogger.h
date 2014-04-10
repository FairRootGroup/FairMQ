/**
 * FairMQLogger.h
 *
 * @since 2012-12-04
 * @author D. Klein, A. Rybalchenko
 */

#ifndef FAIRMQLOGGER_H_
#define FAIRMQLOGGER_H_

#include <sstream>
#include <sys/time.h>
#include <iostream>
#include <iomanip>
#include <ctime>

#include "FairMQLogger.h"

using std::ostringstream;

class FairMQLogger
{
  public:
    enum
    {
        DEBUG,
        INFO,
        ERROR,
        STATE
    };
    FairMQLogger();
    virtual ~FairMQLogger();
    ostringstream& Log(int type);

  private:
    ostringstream os;
};

typedef unsigned long long timestamp_t;

timestamp_t get_timestamp();

#define LOG(type) FairMQLogger().Log(FairMQLogger::type)

#endif /* FAIRMQLOGGER_H_ */
