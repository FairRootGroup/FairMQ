/**
 * FairMQSampler.h
 *
 * @since 2012-09-27
 * @author D. Klein, A. Rybalchenko
 */

#ifndef FAIRMQSAMPLER_H_
#define FAIRMQSAMPLER_H_

#include <string>
#include "FairRunAna.h"
#include "FairTask.h"
#include "FairMQDevice.h"
#include "FairMQSamplerTask.h"
#include "TString.h"


/**
 * Reads simulated digis from a root file and samples the digi as a time-series UDP stream.
 * Must be initialized with the filename to the root file and the name of the sub-detector
 * branch, whose digis should be streamed.
 *
 * The purpose of this class is to provide a data source of digis very similar to the
 * future detector output at the point where the detector is connected to the online
 * computing farm. For the development of online analysis algorithms, it is very important
 * to simulate the future detector output as realistic as possible to evaluate the
 * feasibility and quality of the various possible online analysis features.
 */
class FairMQSampler: public FairMQDevice
{
  public:
    enum {
      InputFile = FairMQDevice::Last,
      Branch,
      ParFile,
      EventRate
    };
    FairMQSampler();
    virtual ~FairMQSampler();

    void ResetEventCounter();
    virtual void ListenToCommands();
    virtual void SetProperty(const Int_t& key, const TString& value, const Int_t& slot = 0);
    virtual TString GetProperty(const Int_t& key, const TString& default_ = "", const Int_t& slot = 0);
    virtual void SetProperty(const Int_t& key, const Int_t& value, const Int_t& slot = 0);
    virtual Int_t GetProperty(const Int_t& key, const Int_t& default_ = 0, const Int_t& slot = 0);
  protected:
    FairRunAna* fFairRunAna;
    Int_t fNumEvents;
    FairMQSamplerTask* fSamplerTask;
    TString fInputFile; // Filename of a root file containing the simulated digis.
    TString fParFile;
    TString fBranch; // The name of the sub-detector branch to stream the digis from.
    Int_t fEventRate;
    Int_t fEventCounter;
    virtual void Init();
    virtual void Run();

};

#endif /* FAIRMQSAMPLER_H_ */
