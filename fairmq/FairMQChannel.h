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
#include <atomic>

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
    /// Default constructor
    FairMQChannel();

    /// Constructor
    /// @param type Socket type (push/pull/pub/sub/spub/xsub/pair/req/rep/dealer/router/)
    /// @param method Socket method (bind/connect)
    /// @param address Network address to bind/connect to (e.g. "tcp://127.0.0.1:5555" or "ipc://abc")
    FairMQChannel(const std::string& type, const std::string& method, const std::string& address);

    /// Copy Constructor
    FairMQChannel(const FairMQChannel&);

    /// Assignment operator
    FairMQChannel& operator=(const FairMQChannel&);

    /// Default destructor
    virtual ~FairMQChannel();

    /// Get channel name
    /// @return Returns full channel name (e.g. "data[0]")
    std::string GetChannelName() const;

    /// Get channel prefix
    /// @return Returns channel prefix (e.g. "data")
    std::string GetChannelPrefix() const;

    /// Get socket type
    /// @return Returns socket type (push/pull/pub/sub/spub/xsub/pair/req/rep/dealer/router/)
    std::string GetType() const;

    /// Get socket method
    /// @return Returns socket method (bind/connect)
    std::string GetMethod() const;

    /// Get socket address (e.g. "tcp://127.0.0.1:5555" or "ipc://abc")
    /// @return Returns socket type (e.g. "tcp://127.0.0.1:5555" or "ipc://abc")
    std::string GetAddress() const;

    /// Get socket send buffer size (in number of messages)
    /// @return Returns socket send buffer size (in number of messages)
    int GetSndBufSize() const;

    /// Get socket receive buffer size (in number of messages)
    /// @return Returns socket receive buffer size (in number of messages)
    int GetRcvBufSize() const;

    /// Get socket kernel transmit send buffer size (in bytes)
    /// @return Returns socket kernel transmit send buffer size (in bytes)
    int GetSndKernelSize() const;

    /// Get socket kernel transmit receive buffer size (in bytes)
    /// @return Returns socket kernel transmit receive buffer size (in bytes)
    int GetRcvKernelSize() const;

    /// Get socket rate logging interval (in seconds)
    /// @return Returns socket rate logging interval (in seconds)
    int GetRateLogging() const;

    /// Set socket type
    /// @param type Socket type (push/pull/pub/sub/spub/xsub/pair/req/rep/dealer/router/)
    void UpdateType(const std::string& type);

    /// Set socket method
    /// @param method Socket method (bind/connect)
    void UpdateMethod(const std::string& method);

    /// Set socket address
    /// @param address Socket address (e.g. "tcp://127.0.0.1:5555" or "ipc://abc")
    void UpdateAddress(const std::string& address);

    /// Set socket send buffer size
    /// @param sndBufSize Socket send buffer size (in number of messages)
    void UpdateSndBufSize(const int sndBufSize);

    /// Set socket receive buffer size
    /// @param rcvBufSize Socket receive buffer size (in number of messages)
    void UpdateRcvBufSize(const int rcvBufSize);

    /// Set socket kernel transmit send buffer size (in bytes)
    /// @param sndKernelSize Socket send buffer size (in bytes)
    void UpdateSndKernelSize(const int sndKernelSize);

    /// Set socket kernel transmit receive buffer size (in bytes)
    /// @param rcvKernelSize Socket receive buffer size (in bytes)
    void UpdateRcvKernelSize(const int rcvKernelSize);

    /// Set socket rate logging interval (in seconds)
    /// @param rateLogging Socket rate logging interval (in seconds)
    void UpdateRateLogging(const int rateLogging);

    /// Checks if the configured channel settings are valid (checks the validity parameter, without running full validation (as oposed to ValidateChannel()))
    /// @return true if channel settings are valid, false otherwise.
    bool IsValid() const;

    /// Validates channel configuration
    /// @return true if channel settings are valid, false otherwise.
    bool ValidateChannel();

    /// Resets the channel (requires validation to be used again).
    void ResetChannel();

    FairMQSocket* fSocket;

    /// Sends a message to the socket queue.
    /// @details Send method attempts to send a message by
    /// putting it in the output queue. If the queue is full or queueing is not possible
    /// for some other reason (e.g. no peers connected for a binding socket), the method blocks.
    ///
    /// @param msg Constant reference of unique_ptr to a FairMQMessage
    /// @return Number of bytes that have been queued. -2 If queueing was not possible or timed out.
    /// In case of errors, returns -1.
    int Send(const std::unique_ptr<FairMQMessage>& msg, int sndTimeoutInMs = -1) const;

    /// Sends a message in non-blocking mode.
    /// @details SendAsync method attempts to send a message without blocking by
    /// putting it in the queue.
    ///
    /// @param msg Constant reference of unique_ptr to a FairMQMessage
    /// @return Number of bytes that have been queued. If queueing failed due to
    /// full queue or no connected peers (when binding), returns -2.
    /// In case of errors, returns -1.
    inline int SendAsync(const std::unique_ptr<FairMQMessage>& msg) const
    {
        return fSocket->Send(msg.get(), fNoBlockFlag);
    }

    /// Queues the current message as a part of a multi-part message
    /// @details SendPart method queues the provided message as a part of a multi-part message.
    /// The actual transfer over the network is initiated once final part has been queued with the Send() or SendAsync() methods.
    ///
    /// @param msg Constant reference of unique_ptr to a FairMQMessage
    /// @return Number of bytes that have been queued. -2 If queueing was not possible.
    /// In case of errors, returns -1.
    inline int SendPart(const std::unique_ptr<FairMQMessage>& msg) const
    {
        return fSocket->Send(msg.get(), fSndMoreFlag);
    }

    /// Queues the current message as a part of a multi-part message without blocking
    /// @details SendPart method queues the provided message as a part of a multi-part message without blocking.
    /// The actual transfer over the network is initiated once final part has been queued with the Send() or SendAsync() methods.
    ///
    /// @param msg Constant reference of unique_ptr to a FairMQMessage
    /// @return Number of bytes that have been queued. -2 If queueing was not possible.
    /// In case of errors, returns -1.
    inline int SendPartAsync(const std::unique_ptr<FairMQMessage>& msg) const
    {
        return fSocket->Send(msg.get(), fSndMoreFlag|fNoBlockFlag);
    }

    /// Send a vector of messages
    ///
    /// @param msgVec message vector reference
    /// @return Number of bytes that have been queued. -2 If queueing was not possible or timed out.
    /// In case of errors, returns -1.
    int64_t Send(const std::vector<std::unique_ptr<FairMQMessage>>& msgVec, int sndTimeoutInMs = -1) const;

    /// Sends a vector of message in non-blocking mode.
    /// @details SendAsync method attempts to send a vector of messages without blocking by
    /// putting it them the queue.
    ///
    /// @param msgVec message vector reference
    /// @return Number of bytes that have been queued. If queueing failed due to
    /// full queue or no connected peers (when binding), returns -2. In case of errors, returns -1.
    inline int64_t SendAsync(const std::vector<std::unique_ptr<FairMQMessage>>& msgVec) const
    {
        return fSocket->Send(msgVec, fNoBlockFlag);
    }

    /// Receives a message from the socket queue.
    /// @details Receive method attempts to receive a message from the input queue.
    /// If the queue is empty the method blocks.
    ///
    /// @param msg Constant reference of unique_ptr to a FairMQMessage
    /// @return Number of bytes that have been received. -2 If reading from the queue was not possible or timed out.
    /// In case of errors, returns -1.
    int Receive(const std::unique_ptr<FairMQMessage>& msg, int rcvTimeoutInMs = -1) const;

    /// Receives a message in non-blocking mode.
    ///
    /// @param msg Constant reference of unique_ptr to a FairMQMessage
    /// @return Number of bytes that have been received. If queue is empty, returns -2.
    /// In case of errors, returns -1.
    inline int ReceiveAsync(const std::unique_ptr<FairMQMessage>& msg) const
    {
        return fSocket->Receive(msg.get(), fNoBlockFlag);
    }

    /// Receive a vector of messages
    ///
    /// @param msgVec message vector reference
    /// @return Number of bytes that have been received. -2 If reading from the queue was not possible or timed out.
    /// In case of errors, returns -1.
    int64_t Receive(std::vector<std::unique_ptr<FairMQMessage>>& msgVec, int rcvTimeoutInMs = -1) const;

    /// Receives a vector of messages in non-blocking mode.
    ///
    /// @param msgVec message vector reference
    /// @return Number of bytes that have been received. If queue is empty, returns -2.
    /// In case of errors, returns -1.
    inline int64_t ReceiveAsync(std::vector<std::unique_ptr<FairMQMessage>>& msgVec) const
    {
        return fSocket->Receive(msgVec, fNoBlockFlag);
    }

    /// Checks if the socket is expecting to receive another part of a multipart message.
    /// @return Return true if the socket expects another part of a multipart message and false otherwise.
    bool ExpectsAnotherPart() const;

    // DEPRECATED socket method wrappers with raw pointers and flag checks
    int Send(FairMQMessage* msg, const std::string& flag = "", int sndTimeoutInMs = -1) const;
    int Send(FairMQMessage* msg, const int flags, int sndTimeoutInMs = -1) const;
    int Receive(FairMQMessage* msg, const std::string& flag = "", int rcvTimeoutInMs = -1) const;
    int Receive(FairMQMessage* msg, const int flags, int rcvTimeoutInMs = -1) const;

  private:
    std::string fType;
    std::string fMethod;
    std::string fAddress;
    int fSndBufSize;
    int fRcvBufSize;
    int fSndKernelSize;
    int fRcvKernelSize;
    int fRateLogging;

    std::string fChannelName;
    std::atomic<bool> fIsValid;

    FairMQPoller* fPoller;
    FairMQSocket* fCmdSocket;

    FairMQTransportFactory* fTransportFactory;

    int fNoBlockFlag;
    int fSndMoreFlag;

    bool InitCommandInterface(FairMQTransportFactory* factory, int numIoThreads);

    bool HandleUnblock() const;

    // use static mutex to make the class easily copyable
    // implication: same mutex is used for all instances of the class
    // this does not hurt much, because mutex is used only during initialization with very low contention
    // possible TODO: improve this
    static boost::mutex fChannelMutex;

    static std::atomic<bool> fInterrupted;
};

#endif /* FAIRMQCHANNEL_H_ */
