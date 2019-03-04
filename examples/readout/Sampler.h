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

#ifndef FAIRMQEXAMPLEREGIONSAMPLER_H
#define FAIRMQEXAMPLEREGIONSAMPLER_H

#include <atomic>

#include "FairMQDevice.h"

namespace example_region
{

class Sampler : public FairMQDevice
{
  public:
    Sampler();
    virtual ~Sampler();

  protected:
    virtual void InitTask();
    virtual bool ConditionalRun();
    virtual void ResetTask();

  private:
    int fMsgSize;
    uint64_t fMaxIterations;
    uint64_t fNumIterations;
    FairMQUnmanagedRegionPtr fRegion;
    std::atomic<uint64_t> fNumUnackedMsgs;
};

} // namespace example_region

#endif /* FAIRMQEXAMPLEREGIONSAMPLER_H */
