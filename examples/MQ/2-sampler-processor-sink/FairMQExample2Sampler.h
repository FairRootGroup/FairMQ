/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *              GNU Lesser General Public Licence (LGPL) version 3,             *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQExample2Sampler.h
 *
 * @since 2014-10-10
 * @author A. Rybalchenko
 */

#ifndef FAIRMQEXAMPLE2SAMPLER_H_
#define FAIRMQEXAMPLE2SAMPLER_H_

#include <string>

#include "FairMQDevice.h"

class FairMQExample2Sampler : public FairMQDevice
{
  public:
    FairMQExample2Sampler();
    virtual ~FairMQExample2Sampler();

  protected:
    std::string fText;
    uint64_t fMaxIterations;
    uint64_t fNumIterations;

    virtual void InitTask();
    virtual bool ConditionalRun();
};

#endif /* FAIRMQEXAMPLE2SAMPLER_H_ */
