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


class FairMQLogger
{
  private:
    static FairMQLogger* instance;
    std::string fBindAddress;
  public:
    enum {
      DEBUG, INFO, ERROR, STATE
    };
    FairMQLogger();
    FairMQLogger(std::string bindAdress);
    virtual ~FairMQLogger();
    void Log(int type, std::string logmsg);
    static FairMQLogger* GetInstance();
    static FairMQLogger* InitInstance(std::string bindAddress);
};

typedef unsigned long long timestamp_t;

static timestamp_t get_timestamp ()
{
  struct timeval now;
  gettimeofday (&now, NULL);
  return now.tv_usec + (timestamp_t)now.tv_sec * 1000000;
}

#endif /* FAIRMQLOGGER_H_ */
