/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQTestReq.h
 *
 * @since 2015-09-05
 * @author A. Rybalchenko
 */

#ifndef FAIRMQTESTREQ_H_
#define FAIRMQTESTREQ_H_

#include "FairMQDevice.h"

class FairMQTestReq : public FairMQDevice
{
  public:
    FairMQTestReq();
    virtual ~FairMQTestReq();

  protected:
    virtual void Run();
};

#endif /* FAIRMQTESTREQ_H_ */
