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
#include <thread>

#include "FairMQDevice.h"

/**
 * Sampler to generate traffic for benchmarking.
 */

class FairMQBenchmarkSampler : public FairMQDevice
{
  public:
    FairMQBenchmarkSampler();
    virtual ~FairMQBenchmarkSampler();

    void PreRun() override;
    void PostRun() override;

    void ResetMsgCounter();

  protected:
    bool fSameMessage;
    int fMsgSize;
    int fMsgCounter;
    int fMsgRate;
    uint64_t fNumIterations;
    uint64_t fMaxIterations;
    std::string fOutChannelName;
    std::thread fResetMsgCounter;

    virtual void InitTask() override;
    virtual void Run() override;
};

#endif /* FAIRMQBENCHMARKSAMPLER_H_ */
