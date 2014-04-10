/**
 * FairMQTransportFactoryZMQ.cxx
 *
 * @since 2014-01-20
 * @author: A. Rybalchenko
 */

#include "zmq.h"

#include "FairMQTransportFactoryZMQ.h"

FairMQTransportFactoryZMQ::FairMQTransportFactoryZMQ()
{
    int major, minor, patch;
    zmq_version(&major, &minor, &patch);
    LOG(INFO) << "Using ZeroMQ library, version: " << major << "." << minor << "." << patch;
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

FairMQSocket* FairMQTransportFactoryZMQ::CreateSocket(const string& type, int num, int numIoThreads)
{
  return new FairMQSocketZMQ(type, num, numIoThreads);
}

FairMQPoller* FairMQTransportFactoryZMQ::CreatePoller(const vector<FairMQSocket*>& inputs)
{
    return new FairMQPollerZMQ(inputs);
}
