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
    virtual void SetProperty(const int& key, const std::string& value, const int& slot = 0);
    virtual std::string GetProperty(const int& key, const std::string& default_ = "", const int& slot = 0);
    virtual void SetProperty(const int& key, const int& value, const int& slot = 0);
    virtual int GetProperty(const int& key, const int& default_ = 0, const int& slot = 0);
  protected:
    FairRunAna* fFairRunAna;
    int fNumEvents;
    FairMQSamplerTask* fSamplerTask;
    std::string fInputFile; // Filename of a root file containing the simulated digis.
    std::string fParFile;
    std::string fBranch; // The name of the sub-detector branch to stream the digis from.
    int fEventRate;
    int fEventCounter;
    virtual void Init();
    virtual void Run();

};

#endif /* FAIRMQSAMPLER_H_ */
