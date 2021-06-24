/********************************************************************************
 * Copyright (C) 2014-2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIRMQCHANNEL_H_
#define FAIRMQCHANNEL_H_

#include <FairMQTransportFactory.h>
#include <FairMQUnmanagedRegion.h>
#include <FairMQSocket.h>
#include <fairmq/Transports.h>
#include <FairMQParts.h>
#include <fairmq/Properties.h>
#include <FairMQMessage.h>

#include <string>
#include <memory> // unique_ptr, shared_ptr
#include <vector>
#include <mutex>
#include <stdexcept>
#include <utility> // std::move
#include <cstdint> // int64_t

/**
 * @class FairMQChannel FairMQChannel.h <FairMQChannel.h>
 * @brief Wrapper class for FairMQSocket and related methods
 *
 * The class is not thread-safe.
 */
class FairMQChannel
{
    friend class FairMQDevice;

  public:
    /// Default constructor
    FairMQChannel();

    /// Constructor
    /// @param name Channel name
    FairMQChannel(const std::string& name);

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

    /// Constructor
    /// @param name Channel name
    /// @param type Socket type (push/pull/pub/sub/spub/xsub/pair/req/rep/dealer/router/)
    /// @param method Socket method (bind/connect)
    /// @param address Network address to bind/connect to (e.g. "tcp://127.0.0.1:5555" or "ipc://abc")
    /// @param factory TransportFactory
    FairMQChannel(std::string name, std::string type, std::string method, std::string address, std::shared_ptr<FairMQTransportFactory> factory);

    FairMQChannel(const std::string& name, int index, const fair::mq::Properties& properties);

    /// Copy Constructor
    FairMQChannel(const FairMQChannel&);

    /// Copy Constructor (with new name)
    FairMQChannel(const FairMQChannel&, std::string name);

    /// Move constructor
    // FairMQChannel(FairMQChannel&&) = delete;

    /// Assignment operator
    FairMQChannel& operator=(const FairMQChannel&);

    /// Move assignment operator
    // FairMQChannel& operator=(FairMQChannel&&) = delete;

    /// Destructor
    virtual ~FairMQChannel() = default;
    // { LOG(warn) << "Destroying channel '" << fName << "'"; }

    struct ChannelConfigurationError : std::runtime_error { using std::runtime_error::runtime_error; };

    FairMQSocket& GetSocket() const { assert(fSocket); return *fSocket; }

    bool Bind(const std::string& address)
    {
        fMethod = "bind";
        fAddress = address;
        return fSocket->Bind(address);
    }

    bool Connect(const std::string& address)
    {
        fMethod = "connect";
        fAddress = address;
        return fSocket->Connect(address);
    }

    /// Get channel name
    /// @return Returns full channel name (e.g. "data[0]")
    std::string GetName() const { return fName; }

    /// Get channel prefix
    /// @return Returns channel prefix (e.g. "data" in "data[0]")
    std::string GetPrefix() const
    {
        std::string prefix = fName;
        prefix = prefix.erase(fName.rfind('['));
        return prefix;
    }

    /// Get channel index
    /// @return Returns channel index (e.g. 0 in "data[0]")
    std::string GetIndex() const
    {
        std::string indexStr = fName;
        indexStr.erase(indexStr.rfind(']'));
        indexStr.erase(0, indexStr.rfind('[') + 1);
        return indexStr;
    }

    /// Get socket type
    /// @return Returns socket type (push/pull/pub/sub/spub/xsub/pair/req/rep/dealer/router/)
    std::string GetType() const { return fType; }

    /// Get socket method
    /// @return Returns socket method (bind/connect)
    std::string GetMethod() const { return fMethod; }

    /// Get socket address (e.g. "tcp://127.0.0.1:5555" or "ipc://abc")
    /// @return Returns socket address (e.g. "tcp://127.0.0.1:5555" or "ipc://abc")
    std::string GetAddress() const { return fAddress; }

    /// Get channel transport name ("default", "zeromq" or "shmem")
    /// @return Returns channel transport name (e.g. "default", "zeromq" or "shmem")
    std::string GetTransportName() const { return fair::mq::TransportName(fTransportType); }

    /// Get channel transport type
    /// @return Returns channel transport type
    fair::mq::Transport GetTransportType() const { return fTransportType; }

    /// Get socket send buffer size (in number of messages)
    /// @return Returns socket send buffer size (in number of messages)
    int GetSndBufSize() const { return fSndBufSize; }

    /// Get socket receive buffer size (in number of messages)
    /// @return Returns socket receive buffer size (in number of messages)
    int GetRcvBufSize() const { return fRcvBufSize; }

    /// Get socket kernel transmit send buffer size (in bytes)
    /// @return Returns socket kernel transmit send buffer size (in bytes)
    int GetSndKernelSize() const { return fSndKernelSize; }

    /// Get socket kernel transmit receive buffer size (in bytes)
    /// @return Returns socket kernel transmit receive buffer size (in bytes)
    int GetRcvKernelSize() const { return fRcvKernelSize; }

    /// Get linger duration (in milliseconds)
    /// @return Returns linger duration (in milliseconds)
    int GetLinger() const { return fLinger; }

    /// Get socket rate logging interval (in seconds)
    /// @return Returns socket rate logging interval (in seconds)
    int GetRateLogging() const { return fRateLogging; }

    /// Get start of the port range for automatic binding
    /// @return start of the port range
    int GetPortRangeMin() const { return fPortRangeMin; }

    /// Get end of the port range for automatic binding
    /// @return end of the port range
    int GetPortRangeMax() const { return fPortRangeMax; }

    /// Set automatic binding (pick random port if bind fails)
    /// @return true/false, true if automatic binding is enabled
    bool GetAutoBind() const { return fAutoBind; }

    /// Set channel name
    /// @param name Arbitrary channel name
    void UpdateName(const std::string& name) { fName = name; Invalidate(); }

    /// Set socket type
    /// @param type Socket type (push/pull/pub/sub/spub/xsub/pair/req/rep/dealer/router/)
    void UpdateType(const std::string& type) { fType = type; Invalidate(); }

    /// Set socket method
    /// @param method Socket method (bind/connect)
    void UpdateMethod(const std::string& method) { fMethod = method; Invalidate(); }

    /// Set socket address
    /// @param Socket address (e.g. "tcp://127.0.0.1:5555" or "ipc://abc")
    void UpdateAddress(const std::string& address) { fAddress = address; Invalidate(); }

    /// Set channel transport
    /// @param transport transport string ("default", "zeromq" or "shmem")
    void UpdateTransport(const std::string& transport) { fTransportType = fair::mq::TransportType(transport); Invalidate(); }

    /// Set socket send buffer size
    /// @param sndBufSize Socket send buffer size (in number of messages)
    void UpdateSndBufSize(const int sndBufSize) { fSndBufSize = sndBufSize; Invalidate(); }

    /// Set socket receive buffer size
    /// @param rcvBufSize Socket receive buffer size (in number of messages)
    void UpdateRcvBufSize(const int rcvBufSize) { fRcvBufSize = rcvBufSize; Invalidate(); }

    /// Set socket kernel transmit send buffer size (in bytes)
    /// @param sndKernelSize Socket send buffer size (in bytes)
    void UpdateSndKernelSize(const int sndKernelSize) { fSndKernelSize = sndKernelSize; Invalidate(); }

    /// Set socket kernel transmit receive buffer size (in bytes)
    /// @param rcvKernelSize Socket receive buffer size (in bytes)
    void UpdateRcvKernelSize(const int rcvKernelSize) { fRcvKernelSize = rcvKernelSize; Invalidate(); }

    /// Set linger duration (in milliseconds)
    /// @param duration linger duration (in milliseconds)
    void UpdateLinger(const int duration) { fLinger = duration; Invalidate(); }

    /// Set socket rate logging interval (in seconds)
    /// @param rateLogging Socket rate logging interval (in seconds)
    void UpdateRateLogging(const int rateLogging) { fRateLogging = rateLogging; Invalidate(); }

    /// Set start of the port range for automatic binding
    /// @param minPort start of the port range
    void UpdatePortRangeMin(const int minPort) { fPortRangeMin = minPort; Invalidate(); }

    /// Set end of the port range for automatic binding
    /// @param maxPort end of the port range
    void UpdatePortRangeMax(const int maxPort) { fPortRangeMax = maxPort; Invalidate(); }

    /// Set automatic binding (pick random port if bind fails)
    /// @param autobind true/false, true to enable automatic binding
    void UpdateAutoBind(const bool autobind) { fAutoBind = autobind; Invalidate(); }

    /// Checks if the configured channel settings are valid (checks the validity parameter, without running full validation (as oposed to ValidateChannel()))
    /// @return true if channel settings are valid, false otherwise.
    bool IsValid() const { return fValid; }

    /// Validates channel configuration
    /// @return true if channel settings are valid, false otherwise.
    bool Validate();

    void Init();

    bool ConnectEndpoint(const std::string& endpoint);

    bool BindEndpoint(std::string& endpoint);

    /// invalidates the channel (requires validation to be used again).
    void Invalidate() { fValid = false; }

    /// Sends a message to the socket queue.
    /// @param msg Constant reference of unique_ptr to a FairMQMessage
    /// @param sndTimeoutInMs send timeout in ms. -1 will wait forever (or until interrupt (e.g. via state change)), 0 will not wait (return immediately if cannot send)
    /// @return Number of bytes that have been queued, TransferCode::timeout if timed out, TransferCode::error if there was an error, TransferCode::interrupted if interrupted (e.g. by requested state change)
    int64_t Send(FairMQMessagePtr& msg, int sndTimeoutInMs = -1)
    {
        CheckSendCompatibility(msg);
        return fSocket->Send(msg, sndTimeoutInMs);
    }

    /// Receives a message from the socket queue.
    /// @param msg Constant reference of unique_ptr to a FairMQMessage
    /// @param rcvTimeoutInMs receive timeout in ms. -1 will wait forever (or until interrupt (e.g. via state change)), 0 will not wait (return immediately if cannot receive)
    /// @return Number of bytes that have been received, TransferCode::timeout if timed out, TransferCode::error if there was an error, TransferCode::interrupted if interrupted (e.g. by requested state change)
    int64_t Receive(FairMQMessagePtr& msg, int rcvTimeoutInMs = -1)
    {
        CheckReceiveCompatibility(msg);
        return fSocket->Receive(msg, rcvTimeoutInMs);
    }

    /// Send a vector of messages
    /// @param msgVec message vector reference
    /// @param sndTimeoutInMs send timeout in ms. -1 will wait forever (or until interrupt (e.g. via state change)), 0 will not wait (return immediately if cannot send)
    /// @return Number of bytes that have been queued, TransferCode::timeout if timed out, TransferCode::error if there was an error, TransferCode::interrupted if interrupted (e.g. by requested state change)
    int64_t Send(std::vector<FairMQMessagePtr>& msgVec, int sndTimeoutInMs = -1)
    {
        CheckSendCompatibility(msgVec);
        return fSocket->Send(msgVec, sndTimeoutInMs);
    }

    /// Receive a vector of messages
    /// @param msgVec message vector reference
    /// @param rcvTimeoutInMs receive timeout in ms. -1 will wait forever (or until interrupt (e.g. via state change)), 0 will not wait (return immediately if cannot receive)
    /// @return Number of bytes that have been received, TransferCode::timeout if timed out, TransferCode::error if there was an error, TransferCode::interrupted if interrupted (e.g. by requested state change)
    int64_t Receive(std::vector<FairMQMessagePtr>& msgVec, int rcvTimeoutInMs = -1)
    {
        CheckReceiveCompatibility(msgVec);
        return fSocket->Receive(msgVec, rcvTimeoutInMs);
    }

    /// Send FairMQParts
    /// @param parts FairMQParts reference
    /// @param sndTimeoutInMs send timeout in ms. -1 will wait forever (or until interrupt (e.g. via state change)), 0 will not wait (return immediately if cannot send)
    /// @return Number of bytes that have been queued, TransferCode::timeout if timed out, TransferCode::error if there was an error, TransferCode::interrupted if interrupted (e.g. by requested state change)
    int64_t Send(FairMQParts& parts, int sndTimeoutInMs = -1)
    {
        return Send(parts.fParts, sndTimeoutInMs);
    }

    /// Receive FairMQParts
    /// @param parts FairMQParts reference
    /// @param rcvTimeoutInMs receive timeout in ms. -1 will wait forever (or until interrupt (e.g. via state change)), 0 will not wait (return immediately if cannot receive)
    /// @return Number of bytes that have been received, TransferCode::timeout if timed out, TransferCode::error if there was an error, TransferCode::interrupted if interrupted (e.g. by requested state change)
    int64_t Receive(FairMQParts& parts, int rcvTimeoutInMs = -1)
    {
        return Receive(parts.fParts, rcvTimeoutInMs);
    }

    unsigned long GetBytesTx() const { return fSocket->GetBytesTx(); }
    unsigned long GetBytesRx() const { return fSocket->GetBytesRx(); }
    unsigned long GetMessagesTx() const { return fSocket->GetMessagesTx(); }
    unsigned long GetMessagesRx() const { return fSocket->GetMessagesRx(); }

    auto Transport() -> FairMQTransportFactory* { return fTransportFactory.get(); };

    template<typename... Args>
    FairMQMessagePtr NewMessage(Args&&... args)
    {
        return Transport()->CreateMessage(std::forward<Args>(args)...);
    }

    template<typename T>
    FairMQMessagePtr NewSimpleMessage(const T& data)
    {
        return Transport()->NewSimpleMessage(data);
    }

    template<typename T>
    FairMQMessagePtr NewStaticMessage(const T& data)
    {
        return Transport()->NewStaticMessage(data);
    }

    template<typename... Args>
    FairMQUnmanagedRegionPtr NewUnmanagedRegion(Args&&... args)
    {
        return Transport()->CreateUnmanagedRegion(std::forward<Args>(args)...);
    }

    static constexpr fair::mq::Transport DefaultTransportType = fair::mq::Transport::DEFAULT;
    static constexpr const char* DefaultTransportName = "default";
    static constexpr const char* DefaultName = "";
    static constexpr const char* DefaultType = "unspecified";
    static constexpr const char* DefaultMethod = "unspecified";
    static constexpr const char* DefaultAddress = "unspecified";
    static constexpr int DefaultSndBufSize = 1000;
    static constexpr int DefaultRcvBufSize = 1000;
    static constexpr int DefaultSndKernelSize = 0;
    static constexpr int DefaultRcvKernelSize = 0;
    static constexpr int DefaultLinger = 500;
    static constexpr int DefaultRateLogging = 1;
    static constexpr int DefaultPortRangeMin = 22000;
    static constexpr int DefaultPortRangeMax = 23000;
    static constexpr bool DefaultAutoBind = true;

  private:
    std::shared_ptr<FairMQTransportFactory> fTransportFactory;
    fair::mq::Transport fTransportType;
    std::unique_ptr<FairMQSocket> fSocket;

    std::string fName;
    std::string fType;
    std::string fMethod;
    std::string fAddress;
    int fSndBufSize;
    int fRcvBufSize;
    int fSndKernelSize;
    int fRcvKernelSize;
    int fLinger;
    int fRateLogging;
    int fPortRangeMin;
    int fPortRangeMax;
    bool fAutoBind;

    bool fValid;

    bool fMultipart;

    void CheckSendCompatibility(FairMQMessagePtr& msg)
    {
        if (fTransportType != msg->GetType()) {
            if (msg->GetSize() > 0) {
                FairMQMessagePtr msgWrapper(NewMessage(
                    msg->GetData(),
                    msg->GetSize(),
                    [](void* /*data*/, void* _msg) { delete static_cast<FairMQMessage*>(_msg); },
                    msg.get()
                ));
                msg.release();
                msg = move(msgWrapper);
            } else {
                FairMQMessagePtr newMsg(NewMessage());
                msg = move(newMsg);
            }
        }
    }

    void CheckSendCompatibility(std::vector<FairMQMessagePtr>& msgVec)
    {
        for (auto& msg : msgVec) {
            if (fTransportType != msg->GetType()) {
                if (msg->GetSize() > 0) {
                    FairMQMessagePtr msgWrapper(NewMessage(
                        msg->GetData(),
                        msg->GetSize(),
                        [](void* /*data*/, void* _msg) { delete static_cast<FairMQMessage*>(_msg); },
                        msg.get()
                    ));
                    msg.release();
                    msg = move(msgWrapper);
                } else {
                    FairMQMessagePtr newMsg(NewMessage());
                    msg = move(newMsg);
                }
            }
        }
    }

    void CheckReceiveCompatibility(FairMQMessagePtr& msg)
    {
        if (fTransportType != msg->GetType()) {
            FairMQMessagePtr newMsg(NewMessage());
            msg = move(newMsg);
        }
    }

    void CheckReceiveCompatibility(std::vector<FairMQMessagePtr>& msgVec)
    {
        for (auto& msg : msgVec) {
            if (fTransportType != msg->GetType()) {
                FairMQMessagePtr newMsg(NewMessage());
                msg = move(newMsg);
            }
        }
    }

    void InitTransport(std::shared_ptr<FairMQTransportFactory> factory)
    {
        fTransportFactory = factory;
        fTransportType = factory->GetType();
    }
};

#endif /* FAIRMQCHANNEL_H_ */
