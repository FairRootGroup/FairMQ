/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQTestSub.h
 *
 * @since 2015-09-05
 * @author A. Rybalchenko
 */

#ifndef FAIRMQTESTSUB_H_
#define FAIRMQTESTSUB_H_

#include "FairMQDevice.h"

class FairMQTestSub : public FairMQDevice
{
  public:
    FairMQTestSub();
    virtual ~FairMQTestSub();

  protected:
    virtual void Run();
};

#endif /* FAIRMQTESTSUB_H_ */
