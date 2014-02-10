/**
 * FairMQTransportFactoryZMQ.cxx
 *
 * @since 2014-01-20
 * @author: A. Rybalchenko
 */

#include "FairMQTransportFactoryZMQ.h"

FairMQTransportFactoryZMQ::FairMQTransportFactoryZMQ()
{
  LOG(INFO) << "Using ZeroMQ library";
}

FairMQMessage* FairMQTransportFactoryZMQ::CreateMessage()
{
  return new FairMQMessageZMQ();
}

FairMQMessage* FairMQTransportFactoryZMQ::CreateMessage(size_t size)
{
  return new FairMQMessageZMQ(size);
}

FairMQMessage* FairMQTransportFactoryZMQ::CreateMessage(void* data, size_t size)
{
  return new FairMQMessageZMQ(data, size);
}

FairMQSocket* FairMQTransportFactoryZMQ::CreateSocket(const string& type, int num)
{
  return new FairMQSocketZMQ(type, num);
}

FairMQPoller* FairMQTransportFactoryZMQ::CreatePoller(const vector<FairMQSocket*>& inputs)
{
  return new FairMQPollerZMQ(inputs);
}
