/**
 * FairMQSocketNN.h
 *
 * @since 2013-12-05
 * @author A. Rybalchenko
 */

#ifndef FAIRMQSOCKETNN_H_
#define FAIRMQSOCKETNN_H_

#include <nanomsg/nn.h>
#include <nanomsg/pipeline.h>
#include <nanomsg/pubsub.h>

#include "FairMQSocket.h"

class FairMQSocketNN : public FairMQSocket
{
  public:
    FairMQSocketNN(const string& type, int num, int numIoThreads); // numIoThreads is not used in nanomsg.

    virtual string GetId();

    virtual void Bind(const string& address);
    virtual void Connect(const string& address);

    virtual size_t Send(FairMQMessage* msg);
    virtual size_t Receive(FairMQMessage* msg);

    virtual void* GetSocket();
    virtual int GetSocket(int nothing);
    virtual void Close();

    virtual void SetOption(const string& option, const void* value, size_t valueSize);

    unsigned long GetBytesTx();
    unsigned long GetBytesRx();
    unsigned long GetMessagesTx();
    unsigned long GetMessagesRx();

    static int GetConstant(const string& constant);

    virtual ~FairMQSocketNN();

  private:
    int fSocket;
    string fId;
    unsigned long fBytesTx;
    unsigned long fBytesRx;
    unsigned long fMessagesTx;
    unsigned long fMessagesRx;
};

#endif /* FAIRMQSOCKETNN_H_ */
