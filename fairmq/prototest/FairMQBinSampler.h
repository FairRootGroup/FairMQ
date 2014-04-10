/**
 * FairMQBinSampler.h
 *
 * @since 2014-02-24
 * @author A. Rybalchenko
 */

#ifndef FAIRMQBINSAMPLER_H_
#define FAIRMQBINSAMPLER_H_

#include <string>

#include "FairMQDevice.h"

struct Content
{
    double a;
    double b;
    int x;
    int y;
    int z;
};

class FairMQBinSampler : public FairMQDevice
{
  public:
    enum
    {
        InputFile = FairMQDevice::Last,
        EventRate,
        EventSize,
        Last
    };
    FairMQBinSampler();
    virtual ~FairMQBinSampler();
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

#endif /* FAIRMQBINSAMPLER_H_ */
