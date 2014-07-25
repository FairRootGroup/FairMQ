/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQSocket.h
 *
 * @since 2012-12-05
 * @author D. Klein, A. Rybalchenko
 */

#ifndef FAIRMQSOCKET_H_
#define FAIRMQSOCKET_H_

#include <string>
#include "FairMQMessage.h"

using std::string;
using std::stringstream;

class FairMQSocket
{
  public:
    virtual string GetId() = 0;

    virtual void Bind(const string& address) = 0;
    virtual void Connect(const string& address) = 0;

    virtual size_t Send(FairMQMessage* msg, const string& flag="") = 0;
    virtual size_t Receive(FairMQMessage* msg, const string& flag="") = 0;

    virtual void* GetSocket() = 0;
    virtual int GetSocket(int nothing) = 0;
    virtual void Close() = 0;

    virtual void SetOption(const string& option, const void* value, size_t valueSize) = 0;
    virtual void GetOption(const string& option, void* value, size_t* valueSize) = 0;

    virtual unsigned long GetBytesTx() = 0;
    virtual unsigned long GetBytesRx() = 0;
    virtual unsigned long GetMessagesTx() = 0;
    virtual unsigned long GetMessagesRx() = 0;

    virtual ~FairMQSocket() {};
};

#endif /* FAIRMQSOCKET_H_ */
