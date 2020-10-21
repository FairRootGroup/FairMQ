/********************************************************************************
 * Copyright (C) 2014-2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_EXPERIMENTALBENCHMARKSAMPLER_H_
#define FAIR_MQ_EXPERIMENTALBENCHMARKSAMPLER_H_

#include "FairMQDevice.h"
#include "tools/RateLimit.h"

#include <fairlogger/Logger.h>

#include <chrono>
#include <cstddef>   // size_t
#include <cstdint>   // uint64_t
#include <cstring>   // memset
#include <string>

/**
 * Sampler to generate traffic for benchmarking.
 */

namespace fair
{
namespace mq
{

class ExperimentalBenchmarkSampler : public FairMQDevice
{
  public:
    ExperimentalBenchmarkSampler()
        : fMemSet(false)
        , fNumBuffers(1)
        , fBufSize(10000)
        , fBufAlignment(0)
        , fMsgRate(0)
        , fNumIterations(0)
        , fMaxIterations(0)
        , fOutChannelName()
    {}

    void InitTask() override
    {
        fMemSet         = fConfig->GetProperty<bool>("memset");
        fNumBuffers     = fConfig->GetProperty<size_t>("num-buffers");
        fBufSize        = fConfig->GetProperty<size_t>("buf-size");
        fBufAlignment   = fConfig->GetProperty<size_t>("buf-alignment");
        fMsgRate        = fConfig->GetProperty<float>("msg-rate");
        fMaxIterations  = fConfig->GetProperty<uint64_t>("max-iterations");
        fOutChannelName = fConfig->GetProperty<std::string>("out-channel");
    }

    void Run() override
    {
        // store the channel reference to avoid traversing the map on every loop iteration
        FairMQChannel& dataOutChannel = fChannels.at(fOutChannelName).at(0);

        LOG(info) << "Starting the benchmark with messages consisting of " << fNumBuffers << " buffer(s) of size " << fBufSize << " and " << fMaxIterations << " iterations.";
        auto tStart = std::chrono::high_resolution_clock::now();

        tools::RateLimiter rateLimiter(fMsgRate);

        while (!NewStatePending()) {
            if (fNumBuffers > 1) {
                Msg msg(fNumBuffers);

                for (size_t i = 0; i < fNumBuffers; ++i) {
                    const auto& buffer = msg.Add(dataOutChannel.NewBuffer(fBufSize, Alignment{fBufAlignment}));
                    if (fMemSet) {
                        std::memset(buffer.GetData(), 0, buffer.GetSize());
                    }
                }

                if (dataOutChannel.Send(std::move(msg)).code == TransferCode::success) {
                    if (fMaxIterations > 0) {
                        if (fNumIterations >= fMaxIterations) {
                            break;
                        }
                    }
                    ++fNumIterations;
                }
            } else {
                Buffer buf(dataOutChannel.NewBuffer(fBufSize, Alignment{fBufAlignment}));
                if (fMemSet) {
                    std::memset(buf.GetData(), 0, buf.GetSize());
                }

                if (dataOutChannel.Send(std::move(buf)).code == TransferCode::success) {
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
    bool fMemSet;
    size_t fNumBuffers;
    size_t fBufSize;
    size_t fBufAlignment;
    float fMsgRate;
    uint64_t fNumIterations;
    uint64_t fMaxIterations;
    std::string fOutChannelName;
};

} // namespace mq
} // namespace fair

#endif /* FAIR_MQ_EXPERIMENTALBENCHMARKSAMPLER_H_ */
