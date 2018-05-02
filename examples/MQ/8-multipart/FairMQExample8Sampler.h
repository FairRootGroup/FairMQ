/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQExample8Sampler.h
 *
 * @since 2014-10-10
 * @author A. Rybalchenko
 */

#ifndef FAIRMQEXAMPLE8SAMPLER_H_
#define FAIRMQEXAMPLE8SAMPLER_H_

#include "FairMQDevice.h"

class FairMQExample8Sampler : public FairMQDevice
{
  public:
    FairMQExample8Sampler();
    virtual ~FairMQExample8Sampler();

  protected:
    virtual bool ConditionalRun();

  private:
    int fCounter;
};

#endif /* FAIRMQEXAMPLE8SAMPLER_H_ */
