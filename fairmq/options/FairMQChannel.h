/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQChannel.h
 *
 * @since 2015-06-02
 * @author A. Rybalchenko
 */

#ifndef FAIRMQCHANNEL_H_
#define FAIRMQCHANNEL_H_

#include <string>

#include "FairMQSocket.h"

class FairMQChannel
{
    friend class FairMQDevice;

  public:
    FairMQChannel();
    FairMQChannel(std::string type, std::string method, std::string protocol, std::string address, std::string port);
    virtual ~FairMQChannel();

    bool ValidateChannel();

    // Wrappers for the socket methods to simplify the usage of channels
    int Send(FairMQMessage* msg, const std::string& flag="");
    int Send(FairMQMessage* msg, const int flags);
    int Receive(FairMQMessage* msg, const std::string& flag="");
    int Receive(FairMQMessage* msg, const int flags);

    std::string fType;
    std::string fMethod;
    std::string fProtocol;
    std::string fAddress;
    std::string fPort;
    int fSndBufSize;
    int fRcvBufSize;
    int fRateLogging;

  private:
    FairMQSocket* fSocket;
    bool fIsValid;
    std::string fChannelName;
};

#endif /* FAIRMQCHANNEL_H_ */