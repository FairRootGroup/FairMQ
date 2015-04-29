/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
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
#include <stdint.h>

#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/timer/timer.hpp>

#include "FairMQDevice.h"
#include "FairMQLogger.h"

/*  GENERIC SAMPLER (data source) MQ-DEVICE */
/*********************************************************************
 * -------------- NOTES -----------------------
 * All policies must have a default constructor
 * Function to define in (parent) policy classes :
 * 
 *  -------- INPUT POLICY (SAMPLER POLICY) --------
 *                SamplerPolicy::InitSampler()
 *        int64_t SamplerPolicy::GetNumberOfEvent()
 * CONTAINER_TYPE SamplerPolicy::GetDataBranch(int64_t eventNr)
 *                SamplerPolicy::SetFileProperties(Args&... args)
 * 
 *  -------- OUTPUT POLICY --------
 *                OutputPolicy::SerializeMsg(CONTAINER_TYPE)
 *                OutputPolicy::SetMessage(FairMQMessage* msg)
 *               
 **********************************************************************/

template <typename SamplerPolicy, typename OutputPolicy>
class GenericSampler : public FairMQDevice, public SamplerPolicy, public OutputPolicy
{
  public:
    enum
    {
        InputFile = FairMQDevice::Last,
        Branch,
        ParFile,
        EventRate
    };

    GenericSampler();
    virtual ~GenericSampler();

    virtual void SetTransport(FairMQTransportFactory* factory);
    void ResetEventCounter();

    template <typename... Args>
    void SetFileProperties(Args&... args)
    {
        SamplerPolicy::SetFileProperties(args...);
    }

    virtual void SetProperty(const int key, const std::string& value);
    virtual std::string GetProperty(const int key, const std::string& default_ = "");
    virtual void SetProperty(const int key, const int value);
    virtual int GetProperty(const int key, const int default_ = 0);

    /**
     * Sends the currently available output of the Sampler Task as part of a multipart message
     * and reinitializes the message to be filled with the next part.
     * This method can be given as a callback to the SamplerTask.
     * The final message part must be sent with normal Send method.
     */
    // temporary disabled
    //void SendPart();

  void SetContinuous(bool flag);

  protected:
    virtual void InitTask();
    virtual void Run();

    int64_t fNumEvents;
    int fEventRate;
    int fEventCounter;
    bool fContinuous;
    std::string fInputFile; // Filename of a root file containing the simulated digis.
    std::string fParFile;
    std::string fBranch; // The name of the sub-detector branch to stream the digis from.
};

#include "GenericSampler.tpl"

#endif /* GENERICSAMPLER_H */

