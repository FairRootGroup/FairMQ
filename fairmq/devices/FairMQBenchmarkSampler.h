/********************************************************************************
 * Copyright (C) 2014-2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIRMQBENCHMARKSAMPLER_H_
#define FAIRMQBENCHMARKSAMPLER_H_

#include <string>
#include <thread>
#include <atomic>

#include "FairMQDevice.h"

/**
 * Sampler to generate traffic for benchmarking.
 */

class FairMQBenchmarkSampler : public FairMQDevice
{
  public:
    FairMQBenchmarkSampler();
    virtual ~FairMQBenchmarkSampler();

  protected:
    bool fMultipart;
    size_t fNumParts;
    size_t fMsgSize;
    std::atomic<int> fMsgCounter;
    float fMsgRate;
    uint64_t fNumIterations;
    uint64_t fMaxIterations;
    std::string fOutChannelName;

    virtual void InitTask() override;
    virtual void Run() override;
};

#endif /* FAIRMQBENCHMARKSAMPLER_H_ */
