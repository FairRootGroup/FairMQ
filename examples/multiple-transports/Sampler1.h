/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIRMQEXAMPLEMULTIPLETRANSPORTSSAMPLER1_H
#define FAIRMQEXAMPLEMULTIPLETRANSPORTSSAMPLER1_H

#include <thread>

#include "FairMQDevice.h"

namespace example_multiple_transports
{

class Sampler1 : public FairMQDevice
{
  public:
    Sampler1();
    virtual ~Sampler1();

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

} // namespace example_multiple_transports

#endif /* FAIRMQEXAMPLEMULTIPLETRANSPORTSSAMPLER1_H */
