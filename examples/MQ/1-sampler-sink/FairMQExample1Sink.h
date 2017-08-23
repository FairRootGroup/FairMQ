/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQExample1Sink.h
 *
 * @since 2014-10-10
 * @author A. Rybalchenko
 */

#ifndef FAIRMQEXAMPLE1SINK_H_
#define FAIRMQEXAMPLE1SINK_H_

#include "FairMQDevice.h"

class FairMQExample1Sink : public FairMQDevice
{
  public:
    FairMQExample1Sink();
    virtual ~FairMQExample1Sink();

  protected:
    virtual void InitTask();
    bool HandleData(FairMQMessagePtr&, int);

  private:
    uint64_t fMaxIterations;
    uint64_t fNumIterations;
};

#endif /* FAIRMQEXAMPLE1SINK_H_ */
