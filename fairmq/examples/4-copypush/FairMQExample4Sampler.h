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

#ifndef FAIRMQEXAMPLESAMPLER_H_
#define FAIRMQEXAMPLESAMPLER_H_

#include <string>

#include "FairMQDevice.h"

class FairMQExample4Sampler : public FairMQDevice
{
  public:
    FairMQExample4Sampler();
    virtual ~FairMQExample4Sampler();

  protected:
    virtual void Run();
};

#endif /* FAIRMQEXAMPLE4SAMPLER_H_ */
