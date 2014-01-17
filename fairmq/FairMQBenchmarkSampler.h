/**
 * FairMQBenchmarkSampler.h
 *
 * @since 2013-04-23
 * @author D. Klein, A. Rybalchenko
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
  public:
    enum {
      InputFile = FairMQDevice::Last,
      EventRate,
      EventSize,
      Last
    };
    FairMQBenchmarkSampler();
    virtual ~FairMQBenchmarkSampler();
    void Log(Int_t intervalInMs);
    void ResetEventCounter();
    virtual void SetProperty(const Int_t& key, const TString& value, const Int_t& slot = 0);
    virtual TString GetProperty(const Int_t& key, const TString& default_ = "", const Int_t& slot = 0);
    virtual void SetProperty(const Int_t& key, const Int_t& value, const Int_t& slot = 0);
    virtual Int_t GetProperty(const Int_t& key, const Int_t& default_ = 0, const Int_t& slot = 0);
  protected:
    Int_t fEventSize;
    Int_t fEventRate;
    Int_t fEventCounter;
    virtual void Init();
    virtual void Run();
};

#endif /* FAIRMQBENCHMARKSAMPLER_H_ */
