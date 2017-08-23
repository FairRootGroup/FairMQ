/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQExample6Sampler.h
 *
 * @since 2014-10-10
 * @author A. Rybalchenko
 */

#ifndef FAIRMQEXAMPLE6SAMPLER_H_
#define FAIRMQEXAMPLE6SAMPLER_H_

#include <string>

#include "FairMQDevice.h"

class FairMQExample6Sampler : public FairMQDevice
{
  public:
    FairMQExample6Sampler();
    virtual ~FairMQExample6Sampler();

  protected:
    std::string fText;
    uint64_t fMaxIterations;
    uint64_t fNumIterations;

    virtual void Run();
    virtual void InitTask();
};

#endif /* FAIRMQEXAMPLE6SAMPLER_H_ */
