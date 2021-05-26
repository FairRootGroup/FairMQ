/********************************************************************************
 * Copyright (C) 2014-2021 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_MERGER_H
#define FAIR_MQ_MERGER_H

#include <FairMQPoller.h>
#include <fairmq/Device.h>

#include <fairlogger/Logger.h>
#include <string>
#include <vector>

namespace fair::mq
{

class Merger : public Device
{
  public:
    Merger()
        : fInChannelName("data-in")
        , fOutChannelName("data-out")
    {}

  protected:
    bool fMultipart = true;
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

} // namespace fair::mq

#endif /* FAIR_MQ_MERGER_H */
