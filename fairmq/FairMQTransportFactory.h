/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQTransportFactory.h
 *
 * @since 2014-01-20
 * @author: A. Rybalchenko
 */

#ifndef FAIRMQTRANSPORTFACTORY_H_
#define FAIRMQTRANSPORTFACTORY_H_

#include <string>

#include "FairMQMessage.h"
#include "FairMQSocket.h"
#include "FairMQPoller.h"
#include "FairMQLogger.h"

class FairMQTransportFactory
{
  public:
    virtual FairMQMessage* CreateMessage() = 0;
    virtual FairMQMessage* CreateMessage(size_t size) = 0;
    virtual FairMQMessage* CreateMessage(void* data, size_t size, fairmq_free_fn *ffn = NULL, void* hint = NULL) = 0;
    virtual FairMQSocket* CreateSocket(const std::string& type, int num, int numIoThreads) = 0;
    virtual FairMQPoller* CreatePoller(const std::vector<FairMQSocket*>& inputs) = 0;

    virtual ~FairMQTransportFactory() {};
};

#endif /* FAIRMQTRANSPORTFACTORY_H_ */
