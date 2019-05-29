/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
#ifndef FAIRMQEXAMPLEREGIONBUILDER_H
#define FAIRMQEXAMPLEREGIONBUILDER_H

#include "FairMQDevice.h"

#include <string>

namespace example_readout
{

class Builder : public FairMQDevice
{
  public:
    Builder()
        : fOutputChannelName()
    {}

    void Init() override
    {
        fOutputChannelName = fConfig->GetProperty<std::string>("output-name");
        OnData("rb", &Builder::HandleData);
    }

    bool HandleData(FairMQMessagePtr& msg, int /*index*/)
    {
        if (Send(msg, fOutputChannelName) < 0) {
            return false;
        }

        return true;
    }

  private:
    std::string fOutputChannelName;
};

} // namespace example_readout

#endif /* FAIRMQEXAMPLEREGIONBUILDER_H */
