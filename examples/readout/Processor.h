/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
#ifndef FAIRMQEXAMPLEREGIONPROCESSOR_H
#define FAIRMQEXAMPLEREGIONPROCESSOR_H

#include "FairMQDevice.h"

namespace example_readout
{

class Processor : public FairMQDevice
{
  public:
    Processor() {
        OnData("bp", &Processor::HandleData);
    }

  protected:
    bool HandleData(FairMQMessagePtr& msg, int /*index*/)
    {
        FairMQMessagePtr msg2(NewMessageFor("ps", 0, msg->GetSize()));
        if (Send(msg2, "ps") < 0) {
            return false;
        }

        return true;
    }
};

} // namespace example_readout

#endif /* FAIRMQEXAMPLEREGIONPROCESSOR_H */
