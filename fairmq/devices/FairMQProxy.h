/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *              GNU Lesser General Public Licence (LGPL) version 3,             *  
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

#include <string>

class FairMQProxy : public FairMQDevice
{
  public:
    FairMQProxy();
    virtual ~FairMQProxy();

  protected:
    bool fMultipart;
    std::string fInChannelName;
    std::string fOutChannelName;

    virtual void Run();
    virtual void InitTask();
};

#endif /* FAIRMQPROXY_H_ */
