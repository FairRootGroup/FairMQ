/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQTestPub.h
 *
 * @since 2015-09-05
 * @author A. Rybalchenko
 */

#ifndef FAIRMQTESTPUB_H_
#define FAIRMQTESTPUB_H_

#include "FairMQDevice.h"

class FairMQTestPub : public FairMQDevice
{
  public:
    FairMQTestPub();
    virtual ~FairMQTestPub();

  protected:
    virtual void Run();
};

#endif /* FAIRMQTESTPUB_H_ */
