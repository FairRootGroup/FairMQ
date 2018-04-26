/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * Sampler.h
 *
 * @since 2014-10-10
 * @author A. Rybalchenko
 */

#ifndef FAIRMQEXAMPLECOPYPUSHSAMPLER_H
#define FAIRMQEXAMPLECOPYPUSHSAMPLER_H

#include "FairMQDevice.h"

#include <stdint.h> // uint64_t

namespace example_copypush
{

class Sampler : public FairMQDevice
{
  public:
    Sampler();
    virtual ~Sampler();

  protected:
    virtual void InitTask();
    virtual bool ConditionalRun();

    int fNumDataChannels;
    uint64_t fCounter;
    uint64_t fMaxIterations;
    uint64_t fNumIterations;
};

} // namespace example_copypush

#endif /* FAIRMQEXAMPLECOPYPUSHSAMPLER_H */
