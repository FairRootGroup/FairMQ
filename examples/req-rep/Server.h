/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * Server.h
 *
 * @since 2014-10-10
 * @author A. Rybalchenko
 */

#ifndef FAIRMQEXAMPLEREQREPSERVER_H
#define FAIRMQEXAMPLEREQREPSERVER_H

#include "FairMQDevice.h"

namespace example_req_rep
{

class Server : public FairMQDevice
{
  public:
    Server();
    virtual ~Server();

  protected:
    virtual void InitTask();
    bool HandleData(FairMQMessagePtr&, int);

  private:
    uint64_t fMaxIterations;
    uint64_t fNumIterations;
};

} // namespace example_req_rep

#endif /* FAIRMQEXAMPLEREQREPSERVER_H */
