/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQExampleServer.h
 *
 * @since 2014-10-10
 * @author A. Rybalchenko
 */

#ifndef FAIRMQEXAMPLESERVER_H_
#define FAIRMQEXAMPLESERVER_H_

#include "FairMQDevice.h"

class FairMQExampleServer : public FairMQDevice
{
  public:
    FairMQExampleServer();
    virtual ~FairMQExampleServer();

    static void CustomCleanup(void *data, void* hint);

  protected:
    virtual void Run();
};

#endif /* FAIRMQEXAMPLESERVER_H_ */
