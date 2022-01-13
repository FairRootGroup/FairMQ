/********************************************************************************
 * Copyright (C) 2014-2021 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_BENCHMARKSAMPLER_H
#define FAIR_MQ_BENCHMARKSAMPLER_H

#include <fairmq/Device.h>
#include <fairmq/tools/RateLimit.h>

#include <chrono>
#include <cstddef>   // size_t
#include <cstdint>   // uint64_t
#include <cstring>   // memset
#include <fairlogger/Logger.h>
#include <string>

namespace fair::mq
{

/**
 * Sampler to generate traffic for benchmarking.
 */

class BenchmarkSampler : public Device
{
  public:
    void InitTask() override
    {
        fMultipart = fConfig->GetProperty<bool>("multipart");
        fMemSet = fConfig->GetProperty<bool>("memset");
        fNumParts = fConfig->GetProperty<size_t>("num-parts");
        fMsgSize = fConfig->GetProperty<size_t>("msg-size");
        fMsgAlignment = fConfig->GetProperty<size_t>("msg-alignment");
        fMsgRate = fConfig->GetProperty<float>("msg-rate");
        fMaxIterations = fConfig->GetProperty<uint64_t>("max-iterations");
        fOutChannelName = fConfig->GetProperty<std::string>("out-channel");
    }

    void Run() override
    {
        // store the channel reference to avoid traversing the map on every loop iteration
        Channel& dataOutChannel = GetChannel(fOutChannelName, 0);

        LOG(info) << "Starting the benchmark with message size of " << fMsgSize << " and " << fMaxIterations << " iterations.";
        auto tStart = std::chrono::high_resolution_clock::now();

        fair::mq::tools::RateLimiter rateLimiter(fMsgRate);

        while (!NewStatePending()) {
            if (fMultipart) {
                Parts parts;

                for (size_t i = 0; i < fNumParts; ++i) {
                    parts.AddPart(dataOutChannel.NewMessage(fMsgSize, fair::mq::Alignment{fMsgAlignment}));
                    if (fMemSet) {
                        std::memset(parts.At(i)->GetData(), 0, parts.At(i)->GetSize());
                    }
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
                MessagePtr msg(dataOutChannel.NewMessage(fMsgSize, fair::mq::Alignment{fMsgAlignment}));
                if (fMemSet) {
                    std::memset(msg->GetData(), 0, msg->GetSize());
                }

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

  protected:
    bool fMultipart = false;
    bool fMemSet = false;
    size_t fNumParts = 1;
    size_t fMsgSize = 10000;
    size_t fMsgAlignment = 0;
    float fMsgRate = 0;
    uint64_t fNumIterations = 0;
    uint64_t fMaxIterations = 0;
    std::string fOutChannelName;
};

} // namespace fair::mq

#endif /* FAIR_MQ_BENCHMARKSAMPLER_H */
