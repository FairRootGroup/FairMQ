/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQExample8Sink.h
 *
 * @since 2014-10-10
 * @author A. Rybalchenko
 */

#ifndef FAIRMQEXAMPLE8SINK_H_
#define FAIRMQEXAMPLE8SINK_H_

#include "FairMQDevice.h"

class FairMQExample8Sink : public FairMQDevice
{
  public:
    FairMQExample8Sink();
    virtual ~FairMQExample8Sink();

  protected:
    virtual void Run();
};

#endif /* FAIRMQEXAMPLE8SINK_H_ */
