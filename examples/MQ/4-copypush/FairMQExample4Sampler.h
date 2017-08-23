/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQExample4Sampler.h
 *
 * @since 2014-10-10
 * @author A. Rybalchenko
 */

#ifndef FAIRMQEXAMPLE4SAMPLER_H_
#define FAIRMQEXAMPLE4SAMPLER_H_

#include "FairMQDevice.h"

#include <stdint.h> // uint64_t

class FairMQExample4Sampler : public FairMQDevice
{
  public:
    FairMQExample4Sampler();
    virtual ~FairMQExample4Sampler();

  protected:
    virtual void InitTask();
    virtual bool ConditionalRun();

    int fNumDataChannels;
    uint64_t fCounter;
    uint64_t fMaxIterations;
    uint64_t fNumIterations;
};

#endif /* FAIRMQEXAMPLE4SAMPLER_H_ */
