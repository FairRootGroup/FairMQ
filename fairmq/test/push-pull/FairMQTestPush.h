/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQTestPush.h
 *
 * @since 2015-09-05
 * @author A. Rybalchenko
 */

#ifndef FAIRMQTESTPUSH_H_
#define FAIRMQTESTPUSH_H_

#include "FairMQDevice.h"

class FairMQTestPush : public FairMQDevice
{
  public:
    FairMQTestPush();
    virtual ~FairMQTestPush();

  protected:
    virtual void Run();
};

#endif /* FAIRMQTESTPUSH_H_ */
