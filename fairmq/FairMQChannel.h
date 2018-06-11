/********************************************************************************
 * Copyright (C) 2014-2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIRMQCHANNEL_H_
#define FAIRMQCHANNEL_H_

#include <string>
#include <memory> // unique_ptr
#include <vector>
#include <atomic>
#include <mutex>

#include <FairMQTransportFactory.h>
#include <FairMQSocket.h>
#include <fairmq/Transports.h>
#include <FairMQLogger.h>
#include <FairMQParts.h>
#include <FairMQMessage.h>

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

    /// Constructor
    /// @param name Channel name
    /// @param type Socket type (push/pull/pub/sub/spub/xsub/pair/req/rep/dealer/router/)
    /// @param factory TransportFactory
    FairMQChannel(const std::string& name, const std::string& type, std::shared_ptr<FairMQTransportFactory> factory);

    /// Copy Constructor
    FairMQChannel(const FairMQChannel&);

    /// Assignment operator
    FairMQChannel& operator=(const FairMQChannel&);

    /// Default destructor
    virtual ~FairMQChannel();

    FairMQSocket const & GetSocket() const;

    auto Bind(const std::string& address) -> bool
    {
        fMethod = "bind";
        fAddress = address;
        return fSocket->Bind(address);
    }

    auto Connect(const std::string& address) -> void
    {
        fMethod = "connect";
        fAddress = address;
        return fSocket->Connect(address);
    }

    /// Get channel name
    /// @return Returns full channel name (e.g. "data[0]")
    std::string GetChannelName() const;

    /// Get channel prefix
    /// @return Returns channel prefix (e.g. "data" in "data[0]")
    std::string GetChannelPrefix() const;

    /// Get channel index
    /// @return Returns channel index (e.g. 0 in "data[0]")
    std::string GetChannelIndex() const;

    /// Get socket type
    /// @return Returns socket type (push/pull/pub/sub/spub/xsub/pair/req/rep/dealer/router/)
    std::string GetType() const;

    /// Get socket method
    /// @return Returns socket method (bind/connect)
    std::string GetMethod() const;

    /// Get socket address (e.g. "tcp://127.0.0.1:5555" or "ipc://abc")
    /// @return Returns socket address (e.g. "tcp://127.0.0.1:5555" or "ipc://abc")
    std::string GetAddress() const;

    /// Get channel transport ("default", "zeromq", "nanomsg" or "shmem")
    /// @return Returns channel transport (e.g. "default", "zeromq", "nanomsg" or "shmem")
    std::string GetTransportName() const;

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
    /// @param Socket address (e.g. "tcp://127.0.0.1:5555" or "ipc://abc")
    void UpdateAddress(const std::string& address);

    /// Set channel transport
    /// @param transport transport string ("default", "zeromq", "nanomsg" or "shmem")
    void UpdateTransport(const std::string& transport);

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

    /// Set channel name
    /// @param name Arbitrary channel name
    void UpdateChannelName(const std::string& name);

    /// Checks if the configured channel settings are valid (checks the validity parameter, without running full validation (as oposed to ValidateChannel()))
    /// @return true if channel settings are valid, false otherwise.
    bool IsValid() const;

    /// Validates channel configuration
    /// @return true if channel settings are valid, false otherwise.
    bool ValidateChannel();

    /// Resets the channel (requires validation to be used again).
    void ResetChannel();

    int Send(FairMQMessagePtr& msg) const;
    int Receive(FairMQMessagePtr& msg) const;

    /// Sends a message to the socket queue.
    /// @details Send method attempts to send a message by
    /// putting it in the output queue. If the queue is full or queueing is not possible
    /// for some other reason (e.g. no peers connected for a binding socket), the method blocks.
    ///
    /// @param msg Constant reference of unique_ptr to a FairMQMessage
    /// @return Number of bytes that have been queued. -2 If queueing was not possible or timed out.
    /// In case of errors, returns -1.
    int Send(FairMQMessagePtr& msg, int sndTimeoutInMs) const;

    /// Receives a message from the socket queue.
    /// @details Receive method attempts to receive a message from the input queue.
    /// If the queue is empty the method blocks.
    ///
    /// @param msg Constant reference of unique_ptr to a FairMQMessage
    /// @return Number of bytes that have been received. -2 If reading from the queue was not possible or timed out.
    /// In case of errors, returns -1.
    int Receive(FairMQMessagePtr& msg, int rcvTimeoutInMs) const;

    /// Sends a message in non-blocking mode.
    /// @details SendAsync method attempts to send a message without blocking by
    /// putting it in the queue.
    ///
    /// @param msg Constant reference of unique_ptr to a FairMQMessage
    /// @return Number of bytes that have been queued. If queueing failed due to
    /// full queue or no connected peers (when binding), returns -2.
    /// In case of errors, returns -1.
    int SendAsync(FairMQMessagePtr& msg) const;

    /// Receives a message in non-blocking mode.
    ///
    /// @param msg Constant reference of unique_ptr to a FairMQMessage
    /// @return Number of bytes that have been received. If queue is empty, returns -2.
    /// In case of errors, returns -1.
    int ReceiveAsync(FairMQMessagePtr& msg) const;

    int64_t Send(std::vector<FairMQMessagePtr>& msgVec) const;
    int64_t Receive(std::vector<FairMQMessagePtr>& msgVec) const;

    /// Send a vector of messages
    ///
    /// @param msgVec message vector reference
    /// @return Number of bytes that have been queued. -2 If queueing was not possible or timed out.
    /// In case of errors, returns -1.
    int64_t Send(std::vector<FairMQMessagePtr>& msgVec, int sndTimeoutInMs) const;

    /// Receive a vector of messages
    ///
    /// @param msgVec message vector reference
    /// @return Number of bytes that have been received. -2 If reading from the queue was not possible or timed out.
    /// In case of errors, returns -1.
    int64_t Receive(std::vector<FairMQMessagePtr>& msgVec, int rcvTimeoutInMs) const;

    /// Sends a vector of message in non-blocking mode.
    /// @details SendAsync method attempts to send a vector of messages without blocking by
    /// putting it them the queue.
    ///
    /// @param msgVec message vector reference
    /// @return Number of bytes that have been queued. If queueing failed due to
    /// full queue or no connected peers (when binding), returns -2. In case of errors, returns -1.
    int64_t SendAsync(std::vector<FairMQMessagePtr>& msgVec) const;

    /// Receives a vector of messages in non-blocking mode.
    ///
    /// @param msgVec message vector reference
    /// @return Number of bytes that have been received. If queue is empty, returns -2.
    /// In case of errors, returns -1.
    int64_t ReceiveAsync(std::vector<FairMQMessagePtr>& msgVec) const;

    int64_t Send(FairMQParts& parts) const
    {
        return Send(parts.fParts);
    }

    int64_t Receive(FairMQParts& parts) const
    {
        return Receive(parts.fParts);
    }

    int64_t Send(FairMQParts& parts, int sndTimeoutInMs) const
    {
        return Send(parts.fParts, sndTimeoutInMs);
    }

    int64_t Receive(FairMQParts& parts, int rcvTimeoutInMs) const
    {
        return Receive(parts.fParts, rcvTimeoutInMs);
    }

    int64_t SendAsync(FairMQParts& parts) const
    {
        return SendAsync(parts.fParts);
    }

    int64_t ReceiveAsync(FairMQParts& parts) const
    {
        return ReceiveAsync(parts.fParts);
    }

    unsigned long GetBytesTx() const;
    unsigned long GetBytesRx() const;
    unsigned long GetMessagesTx() const;
    unsigned long GetMessagesRx() const;

    auto Transport() const -> const FairMQTransportFactory*
    {
        return fTransportFactory.get();
    };

    template<typename... Args>
    FairMQMessagePtr NewMessage(Args&&... args) const
    {
        return Transport()->CreateMessage(std::forward<Args>(args)...);
    }

    template<typename T>
    FairMQMessagePtr NewSimpleMessage(const T& data) const
    {
        return Transport()->NewSimpleMessage(data);
    }

    template<typename T>
    FairMQMessagePtr NewStaticMessage(const T& data) const
    {
        return Transport()->NewStaticMessage(data);
    }

  private:
    std::unique_ptr<FairMQSocket> fSocket;

    std::string fType;
    std::string fMethod;
    std::string fAddress;
    fair::mq::Transport fTransportType;
    int fSndBufSize;
    int fRcvBufSize;
    int fSndKernelSize;
    int fRcvKernelSize;
    int fRateLogging;

    std::string fName;
    std::atomic<bool> fIsValid;

    std::shared_ptr<FairMQTransportFactory> fTransportFactory;

    void CheckSendCompatibility(FairMQMessagePtr& msg) const;
    void CheckSendCompatibility(std::vector<FairMQMessagePtr>& msgVec) const;
    void CheckReceiveCompatibility(FairMQMessagePtr& msg) const;
    void CheckReceiveCompatibility(std::vector<FairMQMessagePtr>& msgVec) const;

    void InitTransport(std::shared_ptr<FairMQTransportFactory> factory);

    // use static mutex to make the class easily copyable
    // implication: same mutex is used for all instances of the class
    // this does not hurt much, because mutex is used only during initialization with very low contention
    // possible TODO: improve this
    static std::mutex fChannelMutex;

    bool fMultipart;
    bool fModified;
    auto SetModified(const bool modified) -> void;
    bool fReset;
};

#endif /* FAIRMQCHANNEL_H_ */
