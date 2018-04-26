/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIRMQEXAMPLEMULTIPLETRANSPORTSSAMPLER2_H
#define FAIRMQEXAMPLEMULTIPLETRANSPORTSSAMPLER2_H

#include "FairMQDevice.h"

namespace example_multiple_transports
{

class Sampler2 : public FairMQDevice
{
  public:
    Sampler2();
    virtual ~Sampler2();

  protected:
    virtual void InitTask();
    virtual bool ConditionalRun();

  private:
    uint64_t fMaxIterations;
    uint64_t fNumIterations;
};

} // namespace example_multiple_transports

#endif /* FAIRMQEXAMPLEMULTIPLETRANSPORTSSAMPLER2_H */
