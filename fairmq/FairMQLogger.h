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

using std::string;
using std::stringstream;

class FairMQLogger
{
  private:
    static FairMQLogger* instance;
    string fBindAddress;
  public:
    enum {
      DEBUG, INFO, ERROR, STATE
    };
    FairMQLogger();
    FairMQLogger(const string& bindAdress); // TODO: check this for const ref
    virtual ~FairMQLogger();
    void Log(int type, const string& logmsg);
    static FairMQLogger* GetInstance();
    static FairMQLogger* InitInstance(const string& bindAddress); // TODO: check this for const ref
};

typedef unsigned long long timestamp_t;

timestamp_t get_timestamp ();

#endif /* FAIRMQLOGGER_H_ */
