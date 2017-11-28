/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQExampleRegionSink.h
 *
 * @since 2014-10-10
 * @author A. Rybalchenko
 */

#ifndef FAIRMQEXAMPLEREGIONSINK_H_
#define FAIRMQEXAMPLEREGIONSINK_H_

#include <string>

#include "FairMQDevice.h"

class FairMQExampleRegionSink : public FairMQDevice
{
  public:
    FairMQExampleRegionSink();
    virtual ~FairMQExampleRegionSink();

  protected:
    virtual void Run();
    virtual void InitTask();

  private:
    uint64_t fMaxIterations;
    uint64_t fNumIterations;
};

#endif /* FAIRMQEXAMPLEREGIONSINK_H_ */
