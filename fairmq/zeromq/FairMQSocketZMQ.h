/**
 * FairMQSocketZMQ.h
 *
 * @since 2012-12-05
 * @author D. Klein, A. Rybalchenko
 */

#ifndef FAIRMQSOCKETZMQ_H_
#define FAIRMQSOCKETZMQ_H_

#include <boost/shared_ptr.hpp>

#include <zmq.h>

#include "FairMQSocket.h"
#include "FairMQContextZMQ.h"


class FairMQSocketZMQ : public FairMQSocket
{
  public:
    FairMQSocketZMQ(const string& type, int num, int numIoThreads);

    virtual string GetId();

    virtual void Bind(const string& address);
    virtual void Connect(const string& address);

    virtual size_t Send(FairMQMessage* msg);
    virtual size_t Receive(FairMQMessage* msg);

    virtual void* GetSocket();
    virtual int GetSocket(int nothing);
    virtual void Close();

    virtual void SetOption(const string& option, const void* value, size_t valueSize);

    virtual unsigned long GetBytesTx();
    virtual unsigned long GetBytesRx();
    virtual unsigned long GetMessagesTx();
    virtual unsigned long GetMessagesRx();

    static int GetConstant(const string& constant);

    virtual ~FairMQSocketZMQ();

  private:
    void* fSocket;
    string fId;
    unsigned long fBytesTx;
    unsigned long fBytesRx;
    unsigned long fMessagesTx;
    unsigned long fMessagesRx;

    static boost::shared_ptr<FairMQContextZMQ> fContext;
};

#endif /* FAIRMQSOCKETZMQ_H_ */
