/********************************************************************************
 * Copyright (C) 2014-2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIRMQBENCHMARKSAMPLER_H_
#define FAIRMQBENCHMARKSAMPLER_H_

#include "../FairMQLogger.h"
#include "FairMQDevice.h"
#include "tools/RateLimit.h"

#include <atomic>
#include <chrono>
#include <cstddef>   // size_t
#include <cstdint>   // uint64_t
#include <string>

/**
 * Sampler to generate traffic for benchmarking.
 */

class FairMQBenchmarkSampler : public FairMQDevice
{
  public:
    FairMQBenchmarkSampler()
        : fMultipart(false)
        , fNumParts(1)
        , fMsgSize(10000)
        , fMsgRate(0)
        , fNumIterations(0)
        , fMaxIterations(0)
        , fOutChannelName()
    {}

    void InitTask() override
    {
        fMultipart = fConfig->GetProperty<bool>("multipart");
        fNumParts = fConfig->GetProperty<size_t>("num-parts");
        fMsgSize = fConfig->GetProperty<size_t>("msg-size");
        fMsgRate = fConfig->GetProperty<float>("msg-rate");
        fMaxIterations = fConfig->GetProperty<uint64_t>("max-iterations");
        fOutChannelName = fConfig->GetProperty<std::string>("out-channel");
    }

    void Run() override
    {
        // store the channel reference to avoid traversing the map on every loop iteration
        FairMQChannel& dataOutChannel = fChannels.at(fOutChannelName).at(0);

        FairMQMessagePtr baseMsg(dataOutChannel.NewMessage(fMsgSize));

        LOG(info) << "Starting the benchmark with message size of " << fMsgSize << " and " << fMaxIterations << " iterations.";
        auto tStart = std::chrono::high_resolution_clock::now();

        fair::mq::tools::RateLimiter rateLimiter(fMsgRate);

        while (!NewStatePending()) {
            if (fMultipart) {
                FairMQParts parts;

                for (size_t i = 0; i < fNumParts; ++i) {
                    parts.AddPart(dataOutChannel.NewMessage(fMsgSize));
                }

                if (dataOutChannel.Send(parts) >= 0) {
                    if (fMaxIterations > 0) {
                        if (fNumIterations >= fMaxIterations) {
                            break;
                        }
                    }
                    ++fNumIterations;
                }
            } else {
                FairMQMessagePtr msg(dataOutChannel.NewMessage(fMsgSize));

                if (dataOutChannel.Send(msg) >= 0) {
                    if (fMaxIterations > 0) {
                        if (fNumIterations >= fMaxIterations) {
                            break;
                        }
                    }
                    ++fNumIterations;
                }
            }

            if (fMsgRate > 0) {
                rateLimiter.maybe_sleep();
            }
        }

        auto tEnd = std::chrono::high_resolution_clock::now();

        LOG(info) << "Done " << fNumIterations << " iterations in " << std::chrono::duration<double, std::milli>(tEnd - tStart).count() << "ms.";
    }

    virtual ~FairMQBenchmarkSampler() {}

  protected:
    bool fMultipart;
    size_t fNumParts;
    size_t fMsgSize;
    std::atomic<int> fMsgCounter;
    float fMsgRate;
    uint64_t fNumIterations;
    uint64_t fMaxIterations;
    std::string fOutChannelName;
};

#endif /* FAIRMQBENCHMARKSAMPLER_H_ */
