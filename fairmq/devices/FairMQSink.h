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

#include <string>
#include <chrono>

#include "../FairMQDevice.h"
#include "../FairMQLogger.h"
#include "../options/FairMQProgOptions.h"

// template<typename OutputPolicy>
class FairMQSink : public FairMQDevice//, public OutputPolicy
{
  public:
    FairMQSink()
        : fMaxIterations(0)
        , fNumIterations(0)
        , fInChannelName()
    {}

    virtual ~FairMQSink()
    {}

  protected:
    uint64_t fMaxIterations;
    uint64_t fNumIterations;
    std::string fInChannelName;

    virtual void InitTask()
    {
        fMaxIterations = fConfig->GetValue<uint64_t>("max-iterations");
        fInChannelName = fConfig->GetValue<std::string>("in-channel");
    }

    virtual void Run()
    {
        // store the channel reference to avoid traversing the map on every loop iteration
        FairMQChannel& dataInChannel = fChannels.at(fInChannelName).at(0);

        LOG(info) << "Starting the benchmark and expecting to receive " << fMaxIterations << " messages.";
        auto tStart = std::chrono::high_resolution_clock::now();

        while (CheckCurrentState(RUNNING))
        {
            FairMQMessagePtr msg(dataInChannel.NewMessage());

            if (dataInChannel.Receive(msg) >= 0)
            {
                if (fMaxIterations > 0)
                {
                    if (fNumIterations >= fMaxIterations)
                    {
                        break;
                    }
                }
                fNumIterations++;
            }
        }

        auto tEnd = std::chrono::high_resolution_clock::now();

        LOG(info) << "Leaving RUNNING state. Received " << fNumIterations << " messages in " << std::chrono::duration<double, std::milli>(tEnd - tStart).count() << "ms.";
    }
};

#endif /* FAIRMQSINK_H_ */
