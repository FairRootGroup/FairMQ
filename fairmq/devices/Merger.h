/********************************************************************************
 * Copyright (C) 2014-2021 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_MERGER_H
#define FAIR_MQ_MERGER_H

#include <fairmq/Poller.h>
#include <fairmq/Device.h>

#include <fairlogger/Logger.h>
#include <string>
#include <vector>

namespace fair::mq
{

class Merger : public Device
{
  protected:
    bool fMultipart = true;
    std::string fInChannelName{"data-in"};
    std::string fOutChannelName{"data-out"};

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
        int numInputs = GetNumSubChannels(fInChannelName);

        std::vector<Channel*> chans;

        for (auto& chan : fChannels.at(fInChannelName)) {
            chans.push_back(&chan);
        }

        PollerPtr poller(NewPoller(chans));

        if (fMultipart) {
            while (!NewStatePending()) {
                poller->Poll(100);

                // Loop over the data input channels.
                for (int i = 0; i < numInputs; ++i) {
                    // Check if the channel has data ready to be received.
                    if (poller->CheckInput(i)) {
                        Parts payload;

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
                        MessagePtr payload(fTransportFactory->CreateMessage());

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
