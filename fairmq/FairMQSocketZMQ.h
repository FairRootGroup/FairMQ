/**
 * FairMQSocketZMQ.h
 *
 * @since 2012-12-05
 * @author D. Klein, A. Rybalchenko
 */

#ifndef FAIRMQSOCKETZMQ_H_
#define FAIRMQSOCKETZMQ_H_

#include <zmq.h>

#include "FairMQSocket.h"
#include "FairMQContext.h"


class FairMQSocketZMQ : public FairMQSocket
{
  public:
    FairMQSocketZMQ(FairMQContext* context, int type, int num);

    virtual std::string GetId();

    virtual void Bind(std::string address);
    virtual void Connect(std::string address);

    virtual size_t Send(FairMQMessage* msg);
    virtual size_t Receive(FairMQMessage* msg);
    virtual void Close();
    virtual void* GetSocket();

    virtual void SetOption(int option, const void* value, size_t valueSize);

    virtual unsigned long GetBytesTx();
    virtual unsigned long GetBytesRx();
    virtual unsigned long GetMessagesTx();
    virtual unsigned long GetMessagesRx();

    static std::string GetTypeString(int type);

    virtual ~FairMQSocketZMQ();

  private:
    void* fSocket;
    std::string fId;
    unsigned long fBytesTx;
    unsigned long fBytesRx;
    unsigned long fMessagesTx;
    unsigned long fMessagesRx;
};

#endif /* FAIRMQSOCKETZMQ_H_ */
