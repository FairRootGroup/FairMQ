/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *              GNU Lesser General Public Licence (LGPL) version 3,             *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQExample5Client.h
 *
 * @since 2014-10-10
 * @author A. Rybalchenko
 */

#ifndef FAIRMQEXAMPLE5CLIENT_H_
#define FAIRMQEXAMPLE5CLIENT_H_

#include <string>

#include "FairMQDevice.h"

class FairMQExample5Client : public FairMQDevice
{
  public:
    FairMQExample5Client();
    virtual ~FairMQExample5Client();

  protected:
    std::string fText;
    uint64_t fMaxIterations;
    uint64_t fNumIterations;

    virtual bool ConditionalRun();
    virtual void InitTask();
};

#endif /* FAIRMQEXAMPLECLIENT_H_ */
