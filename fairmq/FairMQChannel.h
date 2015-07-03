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

#include <boost/thread/mutex.hpp>

#include "FairMQTransportFactory.h"
#include "FairMQSocket.h"
#include "FairMQPoller.h"

class FairMQPoller;
class FairMQTransportFactory;

class FairMQChannel
{
    friend class FairMQDevice;

  public:
    FairMQChannel();
    FairMQChannel(const std::string& type, const std::string& method, const std::string& address);
    virtual ~FairMQChannel();

    std::string GetType() const;
    std::string GetMethod() const;
    std::string GetAddress() const;
    int GetSndBufSize() const;
    int GetRcvBufSize() const;
    int GetRateLogging() const;

    void UpdateType(const std::string& type);
    void UpdateMethod(const std::string& method);
    void UpdateAddress(const std::string& address);
    void UpdateSndBufSize(const int sndBufSize);
    void UpdateRcvBufSize(const int rcvBufSize);
    void UpdateRateLogging(const int rateLogging);

    bool IsValid() const;

    bool ValidateChannel();
    bool InitCommandInterface(FairMQTransportFactory* factory);

    void ResetChannel();

    FairMQSocket* fSocket;

    // Wrappers for the socket methods to simplify the usage of channels
    int Send(FairMQMessage* msg, const std::string& flag = "") const;
    int Send(FairMQMessage* msg, const int flags) const;
    int Receive(FairMQMessage* msg, const std::string& flag = "") const;
    int Receive(FairMQMessage* msg, const int flags) const;

    /// Checks if the socket is expecting to receive another part of a multipart message.
    /// \return Return true if the socket expects another part of a multipart message and false otherwise.
    bool ExpectsAnotherPart() const;

  private:
    std::string fType;
    std::string fMethod;
    std::string fAddress;
    int fSndBufSize;
    int fRcvBufSize;
    int fRateLogging;

    std::string fChannelName;
    bool fIsValid;

    FairMQPoller* fPoller;
    FairMQSocket* fCmdSocket;

    FairMQTransportFactory* fTransportFactory;

    bool HandleCommand() const;

    // use static mutex to make the class easily copyable
    // implication: same mutex is used for all instances of the class
    // this does not hurt much, because mutex is used only during initialization with very low contention
    // possible TODO: improve this
    static boost::mutex channelMutex;
};

#endif /* FAIRMQCHANNEL_H_ */
