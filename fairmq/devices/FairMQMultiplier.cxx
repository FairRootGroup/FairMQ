/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *              GNU Lesser General Public Licence (LGPL) version 3,             *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "FairMQMultiplier.h"

#include "../FairMQLogger.h"
#include "../options/FairMQProgOptions.h"

using namespace std;

FairMQMultiplier::FairMQMultiplier()
    : fMultipart(1)
    , fNumOutputs(0)
    , fInChannelName()
    , fOutChannelNames()
{
}

FairMQMultiplier::~FairMQMultiplier()
{
}

void FairMQMultiplier::InitTask()
{
    fMultipart = fConfig->GetValue<int>("multipart");
    fInChannelName = fConfig->GetValue<string>("in-channel");
    fOutChannelNames = fConfig->GetValue<vector<string>>("out-channel");
    fNumOutputs = fChannels.at(fOutChannelNames.at(0)).size();

    if (fMultipart)
    {
        OnData(fInChannelName, &FairMQMultiplier::HandleMultipartData);
    }
    else
    {
        OnData(fInChannelName, &FairMQMultiplier::HandleSingleData);
    }
}

bool FairMQMultiplier::HandleSingleData(std::unique_ptr<FairMQMessage>& payload, int /*index*/)
{
    for (unsigned int i = 0; i < fOutChannelNames.size() - 1; ++i) // all except last channel
    {
        for (unsigned int j = 0; j < fChannels.at(fOutChannelNames.at(i)).size(); ++j) // all subChannels in a channel
        {
            FairMQMessagePtr msgCopy(fTransportFactory->CreateMessage());
            msgCopy->Copy(*payload);

            Send(msgCopy, fOutChannelNames.at(i), j);
        }
    }

    unsigned int lastChannelSize = fChannels.at(fOutChannelNames.back()).size();

    for (unsigned int i = 0; i < lastChannelSize - 1; ++i) // iterate over all except last subChannels of the last channel
    {
        FairMQMessagePtr msgCopy(fTransportFactory->CreateMessage());
        msgCopy->Copy(*payload);

        Send(msgCopy, fOutChannelNames.back(), i);
    }

    Send(payload, fOutChannelNames.back(), lastChannelSize - 1); // send final message to last subChannel of last channel

    return true;
}

bool FairMQMultiplier::HandleMultipartData(FairMQParts& payload, int /*index*/)
{
    for (unsigned int i = 0; i < fOutChannelNames.size() - 1; ++i) // all except last channel
    {
        for (unsigned int j = 0; j < fChannels.at(fOutChannelNames.at(i)).size(); ++j) // all subChannels in a channel
        {
            FairMQParts parts;

            for (int k = 0; k < payload.Size(); ++k)
            {
                FairMQMessagePtr msgCopy(fTransportFactory->CreateMessage());
                msgCopy->Copy(payload.AtRef(k));
                parts.AddPart(std::move(msgCopy));
            }

            Send(parts, fOutChannelNames.at(i), j);
        }
    }

    unsigned int lastChannelSize = fChannels.at(fOutChannelNames.back()).size();

    for (unsigned int i = 0; i < lastChannelSize - 1; ++i) // iterate over all except last subChannels of the last channel
    {
        FairMQParts parts;

        for (int k = 0; k < payload.Size(); ++k)
        {
            FairMQMessagePtr msgCopy(fTransportFactory->CreateMessage());
            msgCopy->Copy(payload.AtRef(k));
            parts.AddPart(std::move(msgCopy));
        }

        Send(parts, fOutChannelNames.back(), i);
    }

    Send(payload, fOutChannelNames.back(), lastChannelSize - 1); // send final message to last subChannel of last channel

    return true;
}
