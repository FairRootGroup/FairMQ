/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQExample6Broadcaster.h
 *
 * @since 2014-10-10
 * @author A. Rybalchenko
 */

#ifndef FAIRMQEXAMPLE6BROADCASTER_H_
#define FAIRMQEXAMPLE6BROADCASTER_H_

#include "FairMQDevice.h"

class FairMQExample6Broadcaster : public FairMQDevice
{
  public:
    FairMQExample6Broadcaster();
    virtual ~FairMQExample6Broadcaster();

    static void CustomCleanup(void* data, void* hint);

  protected:
    virtual void Run();
};

#endif /* FAIRMQEXAMPLE6BROADCASTER_H_ */
