/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQExample3Sink.h
 *
 * @since 2014-10-10
 * @author A. Rybalchenko
 */

#ifndef FAIRMQEXAMPLE3SINK_H_
#define FAIRMQEXAMPLE3SINK_H_

#include "FairMQDevice.h"

class FairMQExample3Sink : public FairMQDevice
{
  public:
    FairMQExample3Sink();
    virtual ~FairMQExample3Sink();

  protected:
    bool HandleData(FairMQMessagePtr&, int);
};

#endif /* FAIRMQEXAMPLE3SINK_H_ */
