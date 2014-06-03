/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQTransportFactoryZMQ.h
 *
 * @since 2014-01-20
 * @author: A. Rybalchenko
 */

#ifndef FAIRMQTRANSPORTFACTORYZMQ_H_
#define FAIRMQTRANSPORTFACTORYZMQ_H_

#include <vector>

#include "FairMQTransportFactory.h"
#include "FairMQContextZMQ.h"
#include "FairMQMessageZMQ.h"
#include "FairMQSocketZMQ.h"
#include "FairMQPollerZMQ.h"

class FairMQTransportFactoryZMQ : public FairMQTransportFactory
{
  public:
    FairMQTransportFactoryZMQ();

    virtual FairMQMessage* CreateMessage();
    virtual FairMQMessage* CreateMessage(size_t size);
    virtual FairMQMessage* CreateMessage(void* data, size_t size);
    virtual FairMQSocket* CreateSocket(const string& type, int num, int numIoThreads);
    virtual FairMQPoller* CreatePoller(const vector<FairMQSocket*>& inputs);

    virtual ~FairMQTransportFactoryZMQ() {};
};

#endif /* FAIRMQTRANSPORTFACTORYZMQ_H_ */
