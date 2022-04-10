/********************************************************************************
 * Copyright (C) 2014-2022 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
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
        fNumOutputs = GetNumSubChannels(fOutChannelNames.at(0));

        if (fMultipart) {
            OnData(fInChannelName, &Multiplier::HandleMultipartData);
        } else {
            OnData(fInChannelName, &Multiplier::HandleSingleData);
        }
    }


    bool HandleSingleData(std::unique_ptr<Message>& payload, int)
    {
        for (unsigned int i = 0; i < fOutChannelNames.size() - 1; ++i) { // all except last channel
            for (unsigned int j = 0; j < GetNumSubChannels(fOutChannelNames.at(i)); ++j) { // all subChannels in a channel
                MessagePtr msgCopy(fTransportFactory->CreateMessage());
                msgCopy->Copy(*payload);

                Send(msgCopy, fOutChannelNames.at(i), j);
            }
        }

        unsigned int lastChannelSize = GetNumSubChannels(fOutChannelNames.back());

        for (unsigned int i = 0; i < lastChannelSize - 1; ++i) { // iterate over all except last subChannels of the last channel
            MessagePtr msgCopy(fTransportFactory->CreateMessage());
            msgCopy->Copy(*payload);

            Send(msgCopy, fOutChannelNames.back(), i);
        }

        Send(payload, fOutChannelNames.back(), lastChannelSize - 1); // send final message to last subChannel of last channel

        return true;
    }

    bool HandleMultipartData(Parts& payload, int)
    {
        for (unsigned int i = 0; i < fOutChannelNames.size() - 1; ++i) { // all except last channel
            for (unsigned int j = 0; j < GetNumSubChannels(fOutChannelNames.at(i)); ++j) { // all subChannels in a channel
                Parts parts;

                for (unsigned int k = 0; k < payload.Size(); ++k) {
                    MessagePtr msgCopy(fTransportFactory->CreateMessage());
                    msgCopy->Copy(*(payload.At(k)));
                    parts.AddPart(std::move(msgCopy));
                }

                Send(parts, fOutChannelNames.at(i), j);
            }
        }

        unsigned int lastChannelSize = GetNumSubChannels(fOutChannelNames.back());

        for (unsigned int i = 0; i < lastChannelSize - 1; ++i) { // iterate over all except last subChannels of the last channel
            Parts parts;

            for (unsigned int k = 0; k < payload.Size(); ++k) {
                MessagePtr msgCopy(fTransportFactory->CreateMessage());
                msgCopy->Copy(*(payload.At(k)));
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
