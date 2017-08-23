/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIRMQEXAMPLEMTSAMPLER1_H_
#define FAIRMQEXAMPLEMTSAMPLER1_H_

#include <string>
#include <thread>

#include "FairMQDevice.h"

class FairMQExampleMTSampler1 : public FairMQDevice
{
  public:
    FairMQExampleMTSampler1();
    virtual ~FairMQExampleMTSampler1();

  protected:
    virtual void InitTask();
    virtual void PreRun();
    virtual bool ConditionalRun();
    virtual void PostRun();
    void ListenForAcks();

    std::thread fAckListener;
    uint64_t fMaxIterations;
    uint64_t fNumIterations;
};

#endif /* FAIRMQEXAMPLEMTSAMPLER1_H_ */
