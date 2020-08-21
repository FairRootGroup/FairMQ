/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIRMQMULTIPLIER_H_
#define FAIRMQMULTIPLIER_H_

#include "FairMQDevice.h"

#include <string>
#include <vector>

class FairMQMultiplier : public FairMQDevice
{
  public:
    FairMQMultiplier()
        : fMultipart(true)
        , fNumOutputs(0)
        , fInChannelName()
        , fOutChannelNames()
    {}
    ~FairMQMultiplier() {}

  protected:
    bool fMultipart;
    int fNumOutputs;
    std::string fInChannelName;
    std::vector<std::string> fOutChannelNames;

    void InitTask() override
    {
        fMultipart = fConfig->GetProperty<bool>("multipart");
        fInChannelName = fConfig->GetProperty<std::string>("in-channel");
        fOutChannelNames = fConfig->GetProperty<std::vector<std::string>>("out-channel");
        fNumOutputs = fChannels.at(fOutChannelNames.at(0)).size();

        if (fMultipart) {
            OnData(fInChannelName, &FairMQMultiplier::HandleMultipartData);
        } else {
            OnData(fInChannelName, &FairMQMultiplier::HandleSingleData);
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

#endif /* FAIRMQMULTIPLIER_H_ */
