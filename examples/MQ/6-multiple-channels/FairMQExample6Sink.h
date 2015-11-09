/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQExample6Sink.h
 *
 * @since 2014-10-10
 * @author A. Rybalchenko
 */

#ifndef FAIRMQEXAMPLE6SINK_H_
#define FAIRMQEXAMPLE6SINK_H_

#include "FairMQDevice.h"

class FairMQExample6Sink : public FairMQDevice
{
  public:
    FairMQExample6Sink();
    virtual ~FairMQExample6Sink();

  protected:
    virtual void Run();
};

#endif /* FAIRMQEXAMPLE6SINK_H_ */
