/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
#ifndef FAIRMQEXAMPLEREGIONSENDER_H
#define FAIRMQEXAMPLEREGIONSENDER_H

#include "FairMQDevice.h"

#include <string>

namespace example_readout
{

class Sender : public FairMQDevice
{
  public:
    Sender()
        : fInputChannelName()
    {}

    void Init() override
    {
        fInputChannelName = fConfig->GetValue<std::string>("input-name");
        OnData(fInputChannelName, &Sender::HandleData);
    }

    bool HandleData(FairMQMessagePtr& msg, int /*index*/)
    {
        if (Send(msg, "sr") < 0) {
            return false;
        }

        return true;
    }

  private:
    std::string fInputChannelName;
};

} // namespace example_readout

#endif /* FAIRMQEXAMPLEREGIONSENDER_H */
