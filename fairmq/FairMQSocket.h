/*
 * FairMQSocket.h
 *
 *  Created on: Dec 5, 2012
 *      Author: dklein
 */

#ifndef FAIRMQSOCKET_H_
#define FAIRMQSOCKET_H_

#include <zmq.hpp>
#include <string>
#include "FairMQContext.h"
#include "FairMQMessage.h"
#include "Rtypes.h"
#include "TString.h"


class FairMQSocket
{
  private:
    zmq::socket_t* fSocket;
    TString fId;
    ULong_t fBytesTx;
    ULong_t fBytesRx;
    ULong_t fMessagesTx;
    ULong_t fMessagesRx;
  public:
    const static TString TCP, IPC, INPROC;
    FairMQSocket(FairMQContext* context, Int_t type, Int_t num);
    virtual ~FairMQSocket();
    TString GetId();
    static TString GetTypeString(Int_t type);
    Bool_t Send(FairMQMessage* msg);
    Bool_t Receive(FairMQMessage* msg);
    void Close();
    Bool_t Bind(TString address);
    Bool_t Connect(TString address);
    zmq::socket_t* GetSocket();
    ULong_t GetBytesTx();
    ULong_t GetBytesRx();
    ULong_t GetMessagesTx();
    ULong_t GetMessagesRx();

};

#endif /* FAIRMQSOCKET_H_ */
