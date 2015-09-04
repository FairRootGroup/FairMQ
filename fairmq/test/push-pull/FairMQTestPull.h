/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQTestPull.h
 *
 * @since 2015-09-05
 * @author A. Rybalchenko
 */

#ifndef FAIRMQTESTPULL_H_
#define FAIRMQTESTPULL_H_

#include "FairMQDevice.h"

class FairMQTestPull : public FairMQDevice
{
  public:
    FairMQTestPull();
    virtual ~FairMQTestPull();

  protected:
    virtual void Run();
};

#endif /* FAIRMQTESTPULL_H_ */
