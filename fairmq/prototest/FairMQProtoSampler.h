/**
 * FairMQProtoSampler.h
 *
 * @since 2013-04-23
 * @author D. Klein, A. Rybalchenko
 */

#ifndef FAIRMQPROTOSAMPLER_H_
#define FAIRMQPROTOSAMPLER_H_

#include <string>

#include "FairMQDevice.h"

class FairMQProtoSampler: public FairMQDevice
{
  public:
    enum {
      InputFile = FairMQDevice::Last,
      EventRate,
      EventSize,
      Last
    };
    FairMQProtoSampler();
    virtual ~FairMQProtoSampler();
    void Log(int intervalInMs);
    void ResetEventCounter();
    virtual void SetProperty(const int key, const string& value, const int slot = 0);
    virtual string GetProperty(const int key, const string& default_ = "", const int slot = 0);
    virtual void SetProperty(const int key, const int value, const int slot = 0);
    virtual int GetProperty(const int key, const int default_ = 0, const int slot = 0);

  protected:
    int fEventSize;
    int fEventRate;
    int fEventCounter;
    virtual void Init();
    virtual void Run();
};

#endif /* FAIRMQPROTOSAMPLER_H_ */
