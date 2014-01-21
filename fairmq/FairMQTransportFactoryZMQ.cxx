/**
 * FairMQTransportFactoryZMQ.cxx
 *
 * @since 2014-01-20
 * @author: A. Rybalchenko
 */

#include "FairMQTransportFactoryZMQ.h"

FairMQTransportFactoryZMQ::FairMQTransportFactoryZMQ()
{
    
}

FairMQMessage* FairMQTransportFactoryZMQ::CreateMessage()
{
    return new FairMQMessageZMQ();
}

FairMQSocket* FairMQTransportFactoryZMQ::CreateSocket(FairMQContext* context, int type, int num)
{
    return new FairMQSocketZMQ(context, type, num);
}