/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQTestRep.h
 *
 * @since 2015-09-05
 * @author A. Rybalchenko
 */

#ifndef FAIRMQTESTREP_H_
#define FAIRMQTESTREP_H_

#include "FairMQDevice.h"
 
class FairMQTestRep : public FairMQDevice
{
  public:
    FairMQTestRep();
    virtual ~FairMQTestRep();

  protected:
    virtual void Run();
};

#endif /* FAIRMQTESTREP_H_ */
