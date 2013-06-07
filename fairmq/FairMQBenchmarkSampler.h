/*
 * FairMQBenchmarkSampler.h
 *
 *  Created on: Apr 23, 2013
 *      Author: dklein
 */

#ifndef FAIRMQBENCHMARKSAMPLER_H_
#define FAIRMQBENCHMARKSAMPLER_H_

#include <string>
#include "FairMQDevice.h"
#include "TString.h"

/**
 * Sampler to generate traffic for benchmarking.
 */

class FairMQBenchmarkSampler: public FairMQDevice
{
  protected:
    Int_t fEventSize;
    Int_t fEventRate;
    Int_t fEventCounter;
  public:
    enum {
      InputFile = FairMQDevice::Last,
      EventRate,
      EventSize,
      Last
    };
    FairMQBenchmarkSampler();
    virtual ~FairMQBenchmarkSampler();
    virtual void Init();
    virtual void Run();
    void Log(Int_t intervalInMs);
    void* ResetEventCounter();
    static void* callResetEventCounter(void* arg) { return ((FairMQBenchmarkSampler*)arg)->ResetEventCounter(); }
    virtual void SetProperty(Int_t key, TString value, Int_t slot = 0);
    virtual TString GetProperty(Int_t key, TString default_ = "", Int_t slot = 0);
    virtual void SetProperty(Int_t key, Int_t value, Int_t slot = 0);
    virtual Int_t GetProperty(Int_t key, Int_t default_ = 0, Int_t slot = 0);
};

#endif /* FAIRMQBENCHMARKSAMPLER_H_ */
