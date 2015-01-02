/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
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

    virtual int Send(FairMQMessage* msg, const string& flag="");
    virtual int Receive(FairMQMessage* msg, const string& flag="");

    virtual void* GetSocket();
    virtual int GetSocket(int nothing);
    virtual void Close();
    virtual void Terminate();

    virtual void SetOption(const string& option, const void* value, size_t valueSize);
    virtual void GetOption(const string& option, void* value, size_t* valueSize);

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

    /// Copy Constructor
    FairMQSocketZMQ(const FairMQSocketZMQ&);
    FairMQSocketZMQ operator=(const FairMQSocketZMQ&);
};

#endif /* FAIRMQSOCKETZMQ_H_ */
