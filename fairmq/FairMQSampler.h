/*
 * FairMQSampler.h
 *
 *  Created on: Sep 27, 2012
 *      Author: dklein
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
  protected:
    FairRunAna* fFairRunAna;
    FairMQSamplerTask* fSamplerTask;
    TString fInputFile; // Filename of a root file containing the simulated digis.
    TString fParFile;
    TString fBranch; // The name of the sub-detector branch to stream the digis from.
    Int_t fEventRate;
    Int_t fEventCounter;
  public:
    enum {
      InputFile = FairMQDevice::Last,
      Branch,
      ParFile,
      EventRate
    };
    FairMQSampler();
    virtual ~FairMQSampler();
    virtual void Init();
    virtual void Run();
    void Log(Int_t intervalInMs);
    void* ResetEventCounter();
    static void* callResetEventCounter(void* arg) { return ((FairMQSampler*)arg)->ResetEventCounter(); }
    virtual void SetProperty(Int_t key, TString value, Int_t slot = 0);
    virtual TString GetProperty(Int_t key, TString default_ = "", Int_t slot = 0);
    virtual void SetProperty(Int_t key, Int_t value, Int_t slot = 0);
    virtual Int_t GetProperty(Int_t key, Int_t default_ = 0, Int_t slot = 0);
};

#endif /* FAIRMQSAMPLER_H_ */
