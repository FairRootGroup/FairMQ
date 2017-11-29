/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *              GNU Lesser General Public Licence (LGPL) version 3,             *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIRMQEXAMPLEMTSAMPLER2_H_
#define FAIRMQEXAMPLEMTSAMPLER2_H_

#include <string>
#include <thread>

#include "FairMQDevice.h"

class FairMQExampleMTSampler2 : public FairMQDevice
{
  public:
    FairMQExampleMTSampler2();
    virtual ~FairMQExampleMTSampler2();

  protected:
    virtual void InitTask();
    virtual bool ConditionalRun();

  private:
    uint64_t fMaxIterations;
    uint64_t fNumIterations;
};

#endif /* FAIRMQEXAMPLEMTSAMPLER2_H_ */
