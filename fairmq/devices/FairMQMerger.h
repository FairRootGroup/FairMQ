/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQMerger.h
 *
 * @since 2012-12-06
 * @author D. Klein, A. Rybalchenko
 */

#ifndef FAIRMQMERGER_H_
#define FAIRMQMERGER_H_

#include "FairMQDevice.h"
#include "../FairMQPoller.h"
#include "../FairMQLogger.h"

#include <string>
#include <vector>

class FairMQMerger : public FairMQDevice
{
  public:
    FairMQMerger()
        : fMultipart(true)
        , fInChannelName("data-in")
        , fOutChannelName("data-out")
    {}
    ~FairMQMerger() {}

  protected:
    bool fMultipart;
    std::string fInChannelName;
    std::string fOutChannelName;

    void InitTask() override
    {
        fMultipart = fConfig->GetProperty<bool>("multipart");
        fInChannelName = fConfig->GetProperty<std::string>("in-channel");
        fOutChannelName = fConfig->GetProperty<std::string>("out-channel");
    }

    void RegisterChannelEndpoints() override
    {
        RegisterChannelEndpoint(fInChannelName, 1, 10000);
        RegisterChannelEndpoint(fOutChannelName, 1, 1);

        PrintRegisteredChannels();
    }

    void Run() override
    {
        int numInputs = fChannels.at(fInChannelName).size();

        std::vector<FairMQChannel*> chans;

        for (auto& chan : fChannels.at(fInChannelName)) {
            chans.push_back(&chan);
        }

        FairMQPollerPtr poller(NewPoller(chans));

        if (fMultipart) {
            while (!NewStatePending()) {
                poller->Poll(100);

                // Loop over the data input channels.
                for (int i = 0; i < numInputs; ++i) {
                    // Check if the channel has data ready to be received.
                    if (poller->CheckInput(i)) {
                        FairMQParts payload;

                        if (Receive(payload, fInChannelName, i) >= 0) {
                            if (Send(payload, fOutChannelName) < 0) {
                                LOG(debug) << "Transfer interrupted";
                                break;
                            }
                        } else {
                            LOG(debug) << "Transfer interrupted";
                            break;
                        }
                    }
                }
            }
        } else {
            while (!NewStatePending()) {
                poller->Poll(100);

                // Loop over the data input channels.
                for (int i = 0; i < numInputs; ++i) {
                    // Check if the channel has data ready to be received.
                    if (poller->CheckInput(i)) {
                        FairMQMessagePtr payload(fTransportFactory->CreateMessage());

                        if (Receive(payload, fInChannelName, i) >= 0) {
                            if (Send(payload, fOutChannelName) < 0) {
                                LOG(debug) << "Transfer interrupted";
                                break;
                            }
                        } else {
                            LOG(debug) << "Transfer interrupted";
                            break;
                        }
                    }
                }
            }
        }
    }
};

#endif /* FAIRMQMERGER_H_ */
