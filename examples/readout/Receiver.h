/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
#ifndef FAIRMQEXAMPLEREGIONRECEIVER_H
#define FAIRMQEXAMPLEREGIONRECEIVER_H

#include "FairMQDevice.h"

namespace example_readout
{

class Receiver : public FairMQDevice
{
  public:
    Receiver()
        : fMaxIterations(0)
        , fNumIterations(0)
    {}

  protected:
    void InitTask() override
    {
        // Get the fMaxIterations value from the command line options (via fConfig)
        fMaxIterations = fConfig->GetValue<uint64_t>("max-iterations");
    }

    void Run() override
    {
        FairMQChannel& dataInChannel = fChannels.at("sr").at(0);

        while (!NewStatePending()) {
            FairMQMessagePtr msg(dataInChannel.Transport()->CreateMessage());
            dataInChannel.Receive(msg);
            // void* ptr = msg->GetData();

            if (fMaxIterations > 0 && ++fNumIterations >= fMaxIterations) {
                LOG(info) << "Configured maximum number of iterations reached. Leaving RUNNING state.";
                break;
            }
        }
    }

  private:
    uint64_t fMaxIterations;
    uint64_t fNumIterations;
};

} // namespace example_readout

#endif /* FAIRMQEXAMPLEREGIONRECEIVER_H */
