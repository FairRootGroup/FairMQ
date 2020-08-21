/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQSplitter.h
 *
 * @since 2012-12-06
 * @author D. Klein, A. Rybalchenko
 */

#ifndef FAIRMQSPLITTER_H_
#define FAIRMQSPLITTER_H_

#include "FairMQDevice.h"

#include <string>

class FairMQSplitter : public FairMQDevice
{
  public:
    FairMQSplitter()
        : fMultipart(true)
        , fNumOutputs(0)
        , fDirection(0)
        , fInChannelName()
        , fOutChannelName()
    {}
    ~FairMQSplitter() {}

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
            OnData(fInChannelName, &FairMQSplitter::HandleData<FairMQParts>);
        } else {
            OnData(fInChannelName, &FairMQSplitter::HandleData<FairMQMessagePtr>);
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

#endif /* FAIRMQSPLITTER_H_ */
