/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * Client.h
 *
 * @since 2014-10-10
 * @author A. Rybalchenko
 */

#ifndef FAIRMQEXAMPLEREQREPCLIENT_H
#define FAIRMQEXAMPLEREQREPCLIENT_H

#include <string>

#include "FairMQDevice.h"

namespace example_req_rep
{

class Client : public FairMQDevice
{
  public:
    Client();
    virtual ~Client();

  protected:
    std::string fText;
    uint64_t fMaxIterations;
    uint64_t fNumIterations;

    virtual bool ConditionalRun();
    virtual void InitTask();
};

} // namespace example_req_rep

#endif /* FAIRMQEXAMPLEREQREPCLIENT_H */
