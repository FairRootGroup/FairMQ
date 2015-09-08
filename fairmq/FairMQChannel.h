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
#include <memory> // unique_ptr

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
    int Send(const std::unique_ptr<FairMQMessage>& msg) const;

    /// \brief Sends a message in non-blocking mode.
    /// \details SendAsync method attempts to send the message without blocking by
    /// putting it in the queue. If the queue is full or queueing is not possible
    /// for some other reason (e.g. no peers connected for a binding socket), the method returns 0.
    /// 
    /// \param msg Constant reference of unique_ptr to a FairMQMessage
    /// \return Returns the number of bytes that have been queued. If queueing failed due to
    /// full queue or no connected peers (when binding), returns 0. In case of errors, returns -1.
    int SendAsync(const std::unique_ptr<FairMQMessage>& msg) const;
    int SendPart(const std::unique_ptr<FairMQMessage>& msg) const;

    int Receive(const std::unique_ptr<FairMQMessage>& msg) const;
    int ReceiveAsync(const std::unique_ptr<FairMQMessage>& msg) const;

    // DEPRECATED socket method wrappers with raw pointers and flag checks
    int Send(FairMQMessage* msg, const std::string& flag = "") const;
    int Send(FairMQMessage* msg, const int flags) const;
    int Receive(FairMQMessage* msg, const std::string& flag = "") const;
    int Receive(FairMQMessage* msg, const int flags) const;

    /// \brief Checks if the socket is expecting to receive another part of a multipart message.
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

    int fNoBlockFlag;
    int fSndMoreFlag;

    bool HandleUnblock() const;

    // use static mutex to make the class easily copyable
    // implication: same mutex is used for all instances of the class
    // this does not hurt much, because mutex is used only during initialization with very low contention
    // possible TODO: improve this
    static boost::mutex fChannelMutex;
};

#endif /* FAIRMQCHANNEL_H_ */
