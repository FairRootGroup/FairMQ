/********************************************************************************
 * Copyright (C) 2014-2021 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_SPLITTER_H
#define FAIR_MQ_SPLITTER_H

#include <fairmq/Device.h>

#include <string>

namespace fair::mq
{

class Splitter : public Device
{
  public:
    Splitter()
        : fMultipart(true)
        , fNumOutputs(0)
        , fDirection(0)
        , fInChannelName()
        , fOutChannelName()
    {}

  protected:
    bool fMultipart;
    int fNumOutputs;
    int fDirection;
    std::string fInChannelName;
    std::string fOutChannelName;

    void InitTask() override
    {
        fMultipart = fConfig->GetProperty<bool>("multipart");
        fInChannelName = fConfig->GetProperty<std::string>("in-channel");
        fOutChannelName = fConfig->GetProperty<std::string>("out-channel");
        fNumOutputs = fChannels.at(fOutChannelName).size();
        fDirection = 0;

        if (fMultipart) {
            OnData(fInChannelName, &Splitter::HandleData<FairMQParts>);
        } else {
            OnData(fInChannelName, &Splitter::HandleData<FairMQMessagePtr>);
        }
    }

    template<typename T>
    bool HandleData(T& payload, int)
    {
        Send(payload, fOutChannelName, fDirection);

        if (++fDirection >= fNumOutputs) {
            fDirection = 0;
        }

        return true;
    }
};

} // namespace fair::mq

#endif /* FAIR_MQ_SPLITTER_H */
