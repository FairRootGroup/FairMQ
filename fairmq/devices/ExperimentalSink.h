/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_EXSINK_H_
#define FAIR_MQ_EXSINK_H_

#include "../FairMQDevice.h"
#include "../FairMQLogger.h"

#include <chrono>
#include <cstdint> // uint64_t
#include <string>

namespace fair
{
namespace mq
{

class ExperimentalSink : public FairMQDevice
{
  public:
    ExperimentalSink()
        : fMaxIterations(0)
        , fNumIterations(0)
        , fInChannelName()
    {}

    void InitTask() override
    {
        fMaxIterations = fConfig->GetProperty<uint64_t>("max-iterations");
        fInChannelName = fConfig->GetProperty<std::string>("in-channel");
    }

    void Run() override
    {
        // store the channel reference to avoid traversing the map on every loop iteration
        FairMQChannel& dataInChannel = fChannels.at(fInChannelName).at(0);

        LOG(info) << "Starting the benchmark and expecting to receive " << fMaxIterations << " messages.";
        auto tStart = std::chrono::high_resolution_clock::now();

        while (!NewStatePending()) {
            if (dataInChannel.Receive().code == TransferCode::success) {
                if (fMaxIterations > 0) {
                    if (fNumIterations >= fMaxIterations) {
                        LOG(info) << "Configured maximum number of iterations reached.";
                        break;
                    }
                }
                fNumIterations++;
            }
        }

        auto tEnd = std::chrono::high_resolution_clock::now();

        LOG(info) << "Leaving RUNNING state. Received " << fNumIterations << " messages in "
                  << std::chrono::duration<double, std::milli>(tEnd - tStart).count() << "ms.";
    }

  protected:
    uint64_t fMaxIterations;
    uint64_t fNumIterations;
    std::string fInChannelName;
};

} // namespace mq
} // namespace fair

#endif /* FAIR_MQ_EXSINK_H_ */
