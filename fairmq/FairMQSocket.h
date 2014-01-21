/**
 * FairMQSocket.h
 *
 * @since 2012-12-05
 * @author D. Klein, A. Rybalchenko
 */

#ifndef FAIRMQSOCKET_H_
#define FAIRMQSOCKET_H_

#include <string>
#include "FairMQContext.h"
#include "FairMQMessage.h"


class FairMQSocket
{
  public:
    virtual std::string GetId() = 0;

    virtual void Bind(std::string address) = 0;
    virtual void Connect(std::string address) = 0;

    virtual size_t Send(FairMQMessage* msg) = 0;
    virtual size_t Receive(FairMQMessage* msg) = 0;

    virtual void Close() = 0;
    virtual void* GetSocket() = 0;

    virtual void SetOption(int option, const void* value, size_t valueSize) = 0;

    virtual unsigned long GetBytesTx() = 0;
    virtual unsigned long GetBytesRx() = 0;
    virtual unsigned long GetMessagesTx() = 0;
    virtual unsigned long GetMessagesRx() = 0;

    virtual ~FairMQSocket() {};
};

#endif /* FAIRMQSOCKET_H_ */
