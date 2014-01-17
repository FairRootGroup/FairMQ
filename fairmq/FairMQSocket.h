/**
 * FairMQSocket.h
 *
 * @since 2012-12-05
 * @author D. Klein, A. Rybalchenko
 */

#ifndef FAIRMQSOCKET_H_
#define FAIRMQSOCKET_H_

#include <zmq.h>
#include <string>
#include "FairMQContext.h"
#include "FairMQMessage.h"
#include "Rtypes.h"
#include "TString.h"


class FairMQSocket
{
  public:
    FairMQSocket(FairMQContext* context, int type, int num);
    virtual ~FairMQSocket();
    TString GetId();
    static TString GetTypeString(int type);
    size_t Send(FairMQMessage* msg);
    size_t Receive(FairMQMessage* msg);
    void Close();
    void Bind(TString address);
    void Connect(TString address);
    void* GetSocket();

    void SetOption(int option, const void* value, size_t valueSize);

    ULong_t GetBytesTx();
    ULong_t GetBytesRx();
    ULong_t GetMessagesTx();
    ULong_t GetMessagesRx();

  private:
    void* fSocket;
    TString fId;
    ULong_t fBytesTx;
    ULong_t fBytesRx;
    ULong_t fMessagesTx;
    ULong_t fMessagesRx;
};

#endif /* FAIRMQSOCKET_H_ */
