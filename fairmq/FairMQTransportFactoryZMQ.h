/**
 * FairMQTransportFactoryZMQ.h
 *
 * @since 2014-01-20
 * @author: A. Rybalchenko
 */

#ifndef FAIRMQTRANSPORTFACTORYZMQ_H_
#define FAIRMQTRANSPORTFACTORYZMQ_H_

#include "FairMQTransportFactory.h"
#include "FairMQContext.h"
#include "FairMQMessageZMQ.h"
#include "FairMQSocketZMQ.h"

class FairMQTransportFactoryZMQ : public FairMQTransportFactory
{
  public:
    FairMQTransportFactoryZMQ();

    virtual FairMQMessage* CreateMessage();
    virtual FairMQMessage* CreateMessage(size_t size);
    virtual FairMQMessage* CreateMessage(void* data, size_t size);
    virtual FairMQSocket* CreateSocket(FairMQContext* context, int type, int num);

    virtual ~FairMQTransportFactoryZMQ() {};
};

#endif /* FAIRMQTRANSPORTFACTORYZMQ_H_ */
