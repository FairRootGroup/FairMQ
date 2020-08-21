/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQSink.h
 *
 * @since 2013-01-09
 * @author D. Klein, A. Rybalchenko
 */

#ifndef FAIRMQSINK_H_
#define FAIRMQSINK_H_

#include "../FairMQDevice.h"
#include "../FairMQLogger.h"

#include <chrono>
#include <string>

// template<typename OutputPolicy>
class FairMQSink : public FairMQDevice   //, public OutputPolicy
{
  public:
    FairMQSink()
        : fMultipart(false)
        , fMaxIterations(0)
        , fNumIterations(0)
        , fInChannelName()
    {}

    ~FairMQSink() {}

  protected:
    bool fMultipart;
    uint64_t fMaxIterations;
    uint64_t fNumIterations;
    std::string fInChannelName;

    void InitTask() override
    {
        fMultipart = fConfig->GetProperty<bool>("multipart");
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
            if (fMultipart) {
                FairMQParts parts;

                if (dataInChannel.Receive(parts) >= 0) {
                    if (fMaxIterations > 0) {
                        if (fNumIterations >= fMaxIterations) {
                            LOG(info) << "Configured maximum number of iterations reached.";
                            break;
                        }
                    }
                    fNumIterations++;
                }
            } else {
                FairMQMessagePtr msg(dataInChannel.NewMessage());

                if (dataInChannel.Receive(msg) >= 0) {
                    if (fMaxIterations > 0) {
                        if (fNumIterations >= fMaxIterations) {
                            LOG(info) << "Configured maximum number of iterations reached.";
                            break;
                        }
                    }
                    fNumIterations++;
                }
            }
        }

        auto tEnd = std::chrono::high_resolution_clock::now();

        LOG(info) << "Leaving RUNNING state. Received " << fNumIterations << " messages in "
                  << std::chrono::duration<double, std::milli>(tEnd - tStart).count() << "ms.";
    }
};

#endif /* FAIRMQSINK_H_ */
