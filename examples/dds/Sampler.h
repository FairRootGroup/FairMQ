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

#ifndef FAIRMQEXAMPLEDDSSAMPLER_H
#define FAIRMQEXAMPLEDDSSAMPLER_H

#include "FairMQDevice.h"

namespace example_dds
{

class Sampler : public FairMQDevice
{
  public:
    Sampler();
    virtual ~Sampler();
    void InitTask() override;

  protected:
    virtual bool ConditionalRun();

  private:
    uint64_t fIterations;
    uint64_t fCounter;
};

} // namespace example_dds

#endif /* FAIRMQEXAMPLEDDSSAMPLER_H */
