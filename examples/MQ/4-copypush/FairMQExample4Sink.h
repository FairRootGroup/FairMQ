/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQExample4Sink.h
 *
 * @since 2014-10-10
 * @author A. Rybalchenko
 */

#ifndef FAIRMQEXAMPLE4SINK_H_
#define FAIRMQEXAMPLE4SINK_H_

#include "FairMQDevice.h"

class FairMQExample4Sink : public FairMQDevice
{
  public:
    FairMQExample4Sink();
    virtual ~FairMQExample4Sink();

  protected:
    bool HandleData(FairMQMessagePtr&, int);
};

#endif /* FAIRMQEXAMPLE4SINK_H_ */
