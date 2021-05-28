/********************************************************************************
 * Copyright (C) 2014-2021 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_MULTIPLIER_H
#define FAIR_MQ_MULTIPLIER_H

#include <fairmq/Device.h>

#include <string>
#include <vector>

namespace fair::mq
{

class Multiplier : public Device
{
  protected:
    bool fMultipart = true;
    int fNumOutputs = 0;
    std::string fInChannelName;
    std::vector<std::string> fOutChannelNames;

    void InitTask() override
    {
        fMultipart = fConfig->GetProperty<bool>("multipart");
        fInChannelName = fConfig->GetProperty<std::string>("in-channel");
        fOutChannelNames = fConfig->GetProperty<std::vector<std::string>>("out-channel");
        fNumOutputs = fChannels.at(fOutChannelNames.at(0)).size();

        if (fMultipart) {
            OnData(fInChannelName, &Multiplier::HandleMultipartData);
        } else {
            OnData(fInChannelName, &Multiplier::HandleSingleData);
        }
    }


    bool HandleSingleData(std::unique_ptr<FairMQMessage>& payload, int)
    {
        for (unsigned int i = 0; i < fOutChannelNames.size() - 1; ++i) { // all except last channel
            for (unsigned int j = 0; j < fChannels.at(fOutChannelNames.at(i)).size(); ++j) { // all subChannels in a channel
                FairMQMessagePtr msgCopy(fTransportFactory->CreateMessage());
                msgCopy->Copy(*payload);

                Send(msgCopy, fOutChannelNames.at(i), j);
            }
        }

        unsigned int lastChannelSize = fChannels.at(fOutChannelNames.back()).size();

        for (unsigned int i = 0; i < lastChannelSize - 1; ++i) { // iterate over all except last subChannels of the last channel
            FairMQMessagePtr msgCopy(fTransportFactory->CreateMessage());
            msgCopy->Copy(*payload);

            Send(msgCopy, fOutChannelNames.back(), i);
        }

        Send(payload, fOutChannelNames.back(), lastChannelSize - 1); // send final message to last subChannel of last channel

        return true;
    }

    bool HandleMultipartData(FairMQParts& payload, int)
    {
        for (unsigned int i = 0; i < fOutChannelNames.size() - 1; ++i) { // all except last channel
            for (unsigned int j = 0; j < fChannels.at(fOutChannelNames.at(i)).size(); ++j) { // all subChannels in a channel
                FairMQParts parts;

                for (int k = 0; k < payload.Size(); ++k) {
                    FairMQMessagePtr msgCopy(fTransportFactory->CreateMessage());
                    msgCopy->Copy(payload.AtRef(k));
                    parts.AddPart(std::move(msgCopy));
                }

                Send(parts, fOutChannelNames.at(i), j);
            }
        }

        unsigned int lastChannelSize = fChannels.at(fOutChannelNames.back()).size();

        for (unsigned int i = 0; i < lastChannelSize - 1; ++i) { // iterate over all except last subChannels of the last channel
            FairMQParts parts;

            for (int k = 0; k < payload.Size(); ++k) {
                FairMQMessagePtr msgCopy(fTransportFactory->CreateMessage());
                msgCopy->Copy(payload.AtRef(k));
                parts.AddPart(std::move(msgCopy));
            }

            Send(parts, fOutChannelNames.back(), i);
        }

        Send(payload, fOutChannelNames.back(), lastChannelSize - 1); // send final message to last subChannel of last channel

        return true;
    }
};

} // namespace fair::mq

#endif /* FAIR_MQ_MULTIPLIER_H */
