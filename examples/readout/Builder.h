/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIRMQEXAMPLEREGIONBUILDER_H
#define FAIRMQEXAMPLEREGIONBUILDER_H

#include <atomic>

#include "FairMQDevice.h"

namespace example_region
{

class Builder : public FairMQDevice
{
  public:
    Builder() {
        OnData("data1", &Builder::HandleData);
    }
    virtual ~Builder() {}

  protected:
    bool HandleData(FairMQMessagePtr& msg, int /*index*/)
    {
        if (Send(msg, "data2") < 0) {
            return false;
        }

        return true;
    }
};

} // namespace example_region

#endif /* FAIRMQEXAMPLEREGIONBUILDER_H */
