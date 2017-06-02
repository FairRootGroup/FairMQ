/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQExampleRegionSampler.h
 *
 * @since 2014-10-10
 * @author A. Rybalchenko
 */

#ifndef FAIRMQEXAMPLEREGIONSAMPLER_H_
#define FAIRMQEXAMPLEREGIONSAMPLER_H_

#include <string>

#include "FairMQDevice.h"

class FairMQExampleRegionSampler : public FairMQDevice
{
  public:
    FairMQExampleRegionSampler();
    virtual ~FairMQExampleRegionSampler();

  protected:
    int fMsgSize;

    virtual void InitTask();
    virtual void Run();
};

#endif /* FAIRMQEXAMPLEREGIONSAMPLER_H_ */
