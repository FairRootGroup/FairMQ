/* 
 * File:   GenericSampler.h
 * Author: winckler
 *
 * Created on November 24, 2014, 3:30 PM
 */

#ifndef GENERICSAMPLER_H
#define	GENERICSAMPLER_H




#include <vector>
#include <iostream>

#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/timer/timer.hpp>

#include "TList.h"
#include "TObjString.h"
#include "TClonesArray.h"
#include "TROOT.h"


#include "FairMQDevice.h"
#include "FairMQLogger.h"

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

template <typename SamplerPolicy, typename OutputPolicy>
class GenericSampler: public FairMQDevice, public SamplerPolicy, public OutputPolicy
{
    //using SamplerPolicy::GetDataBranch;   // get data from file
    //using OutputPolicy::message;        // serialize method
    
  public:
    enum {
      InputFile = FairMQDevice::Last,
      Branch,
      ParFile,
      EventRate
    };
    GenericSampler();
    virtual ~GenericSampler();
    virtual void SetTransport(FairMQTransportFactory* factory);
    void ResetEventCounter();
    virtual void ListenToCommands();
    
    template <typename... Args>
        void SetFileProperties(Args&... args)
        {
            SamplerPolicy::SetFileProperties(args...);
        }

    virtual void SetProperty(const int key, const string& value, const int slot = 0);
    virtual string GetProperty(const int key, const string& default_ = "", const int slot = 0);
    virtual void SetProperty(const int key, const int value, const int slot = 0);
    virtual int GetProperty(const int key, const int default_ = 0, const int slot = 0);

    /**
     * Sends the currently available output of the Sampler Task as part of a multipart message
     * and reinitializes the message to be filled with the next part.
     * This method can be given as a callback to the SamplerTask.
     * The final message part must be sent with normal Send method.
     */
  void SendPart();

  void SetContinuous(bool flag) { fContinuous = flag; }

protected:
  virtual void Init();
  virtual void Run();

protected:
  string fInputFile; // Filename of a root file containing the simulated digis.
  string fParFile;
  string fBranch; // The name of the sub-detector branch to stream the digis from.
  int fNumEvents;
  int fEventRate;
  int fEventCounter;
  bool fContinuous;
};

#include "GenericSampler.tpl"

#endif	/* GENERICSAMPLER_H */

