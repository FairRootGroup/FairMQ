/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *              GNU Lesser General Public Licence (LGPL) version 3,             *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQExampleMTSink.h
 *
 * @since 2014-10-10
 * @author A. Rybalchenko
 */

#ifndef FAIRMQEXAMPLEMTSINK_H_
#define FAIRMQEXAMPLEMTSINK_H_

#include "FairMQDevice.h"

class FairMQExampleMTSink : public FairMQDevice
{
  public:
    FairMQExampleMTSink();
    virtual ~FairMQExampleMTSink();

  protected:
    virtual void InitTask();
    bool HandleData1(FairMQMessagePtr&, int);
    bool HandleData2(FairMQMessagePtr&, int);
    bool CheckIterations();

  private:
    uint64_t fMaxIterations;
    uint64_t fNumIterations1;
    uint64_t fNumIterations2;
    bool fReceived1;
    bool fReceived2;
};

#endif /* FAIRMQEXAMPLEMTSINK_H_ */
