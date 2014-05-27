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

using std::vector;

class FairMQTransportFactory
{
  public:
    virtual FairMQMessage* CreateMessage() = 0;
    virtual FairMQMessage* CreateMessage(size_t size) = 0;
    virtual FairMQMessage* CreateMessage(void* data, size_t size) = 0;
    virtual FairMQSocket* CreateSocket(const string& type, int num, int numIoThreads) = 0;
    virtual FairMQPoller* CreatePoller(const vector<FairMQSocket*>& inputs) = 0;

    virtual ~FairMQTransportFactory() {};
};

#endif /* FAIRMQTRANSPORTFACTORY_H_ */
