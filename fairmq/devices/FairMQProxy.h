/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQProxy.h
 *
 * @since 2013-10-02
 * @author A. Rybalchenko
 */

#ifndef FAIRMQPROXY_H_
#define FAIRMQPROXY_H_

#include "FairMQDevice.h"

class FairMQProxy : public FairMQDevice
{
  public:
    FairMQProxy();
    virtual ~FairMQProxy();

  protected:
    virtual void Run();
};

#endif /* FAIRMQPROXY_H_ */
