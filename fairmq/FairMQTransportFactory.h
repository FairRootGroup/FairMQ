/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQTransportFactory.h
 *
 * @since 2014-01-20
 * @author: A. Rybalchenko
 */

#ifndef FAIRMQTRANSPORTFACTORY_H_
#define FAIRMQTRANSPORTFACTORY_H_

#include <string>
#include <memory>
#include <vector>
#include <unordered_map>

#include "FairMQMessage.h"
#include "FairMQSocket.h"
#include "FairMQPoller.h"
#include "FairMQLogger.h"
#include "FairMQTransports.h"

class FairMQChannel;
class FairMQProgOptions;

class FairMQTransportFactory
{
  public:
    /// Initialize transport
    virtual void Initialize(const FairMQProgOptions* config) = 0;

    /// Create an empty message (e.g. for receiving)
    virtual FairMQMessagePtr CreateMessage() const = 0;
    /// Create an empty of a specified size
    virtual FairMQMessagePtr CreateMessage(const size_t size) const = 0;
    /// Create a message from a supplied buffer, size and a deallocation callback
    virtual FairMQMessagePtr CreateMessage(void* data, const size_t size, fairmq_free_fn* ffn, void* hint = nullptr) const = 0;

    /// Create a socket
    virtual FairMQSocketPtr CreateSocket(const std::string& type, const std::string& name, const std::string& id = "") const = 0;

    /// Create a poller for all device channels
    virtual FairMQPollerPtr CreatePoller(const std::vector<FairMQChannel>& channels) const = 0;
    /// Create a poller for all device channels
    virtual FairMQPollerPtr CreatePoller(const std::unordered_map<std::string, std::vector<FairMQChannel>>& channelsMap, const std::vector<std::string>& channelList) const = 0;
    /// Create a poller for two sockets
    virtual FairMQPollerPtr CreatePoller(const FairMQSocket& cmdSocket, const FairMQSocket& dataSocket) const = 0;

    /// Get transport type
    virtual FairMQ::Transport GetType() const = 0;

    /// Shutdown transport (stop transfers, get ready for complete shutdown)
    virtual void Shutdown() = 0;
    /// Terminate transport (complete shutdown)
    virtual void Terminate() = 0;

    virtual ~FairMQTransportFactory() {};
};

#endif /* FAIRMQTRANSPORTFACTORY_H_ */
