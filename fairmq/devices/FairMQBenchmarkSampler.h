/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
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

/**
 * Sampler to generate traffic for benchmarking.
 */

class FairMQBenchmarkSampler : public FairMQDevice
{
  public:
    enum
    {
        InputFile = FairMQDevice::Last,
        EventRate,
        EventSize,
        Last
    };
    FairMQBenchmarkSampler();
    virtual ~FairMQBenchmarkSampler();
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

#endif /* FAIRMQBENCHMARKSAMPLER_H_ */
