/**
 * FairMQTransportFactory.h
 *
 * @since 2014-01-20
 * @author: A. Rybalchenko
 */

#ifndef FAIRMQTRANSPORTFACTORY_H_
#define FAIRMQTRANSPORTFACTORY_H_

#include "FairMQMessage.h"
#include "FairMQSocket.h"

class FairMQTransportFactory
{
  public:
    virtual FairMQMessage* CreateMessage() = 0;
    virtual FairMQSocket* CreateSocket(FairMQContext* context, int type, int num) = 0;

    virtual ~FairMQTransportFactory() {};
};

#endif /* FAIRMQTRANSPORTFACTORY_H_ */
