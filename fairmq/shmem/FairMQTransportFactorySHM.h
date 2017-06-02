/********************************************************************************
 * Copyright (C) 2016-2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIRMQTRANSPORTFACTORYSHM_H_
#define FAIRMQTRANSPORTFACTORYSHM_H_

#include "FairMQTransportFactory.h"
#include "FairMQMessageSHM.h"
#include "FairMQSocketSHM.h"
#include "FairMQPollerSHM.h"
#include "FairMQShmCommon.h"
#include <options/FairMQProgOptions.h>
#include "FairMQRegionSHM.h"

#include <vector>
#include <string>
#include <thread>
#include <atomic>

#include <boost/interprocess/sync/named_mutex.hpp>

class FairMQTransportFactorySHM : public FairMQTransportFactory
{
  public:
    FairMQTransportFactorySHM(const std::string& id = "", const FairMQProgOptions* config = nullptr);
    FairMQTransportFactorySHM(const FairMQTransportFactorySHM&) = delete;
    FairMQTransportFactorySHM operator=(const FairMQTransportFactorySHM&) = delete;

    FairMQMessagePtr CreateMessage() const override;
    FairMQMessagePtr CreateMessage(const size_t size) const override;
    FairMQMessagePtr CreateMessage(void* data, const size_t size, fairmq_free_fn* ffn, void* hint = nullptr) const override;
    FairMQMessagePtr CreateMessage(FairMQRegionPtr& region, void* data, const size_t size) const override;

    FairMQSocketPtr CreateSocket(const std::string& type, const std::string& name) const override;

    FairMQPollerPtr CreatePoller(const std::vector<FairMQChannel>& channels) const override;
    FairMQPollerPtr CreatePoller(const std::vector<const FairMQChannel*>& channels) const override;
    FairMQPollerPtr CreatePoller(const std::unordered_map<std::string, std::vector<FairMQChannel>>& channelsMap, const std::vector<std::string>& channelList) const override;
    FairMQPollerPtr CreatePoller(const FairMQSocket& cmdSocket, const FairMQSocket& dataSocket) const override;

    FairMQRegionPtr CreateRegion(const size_t size) const override;

    FairMQ::Transport GetType() const override;

    ~FairMQTransportFactorySHM() override;

  private:
    void SendHeartbeats();
    void StartMonitor();

    static FairMQ::Transport fTransportType;
    void* fContext;
    void* fHeartbeatSocket;
    std::thread fHeartbeatThread;
    std::atomic<bool> fSendHeartbeats;
    boost::interprocess::named_mutex fShMutex;
    fair::mq::shmem::DeviceCounter* fDeviceCounter;
};

#endif /* FAIRMQTRANSPORTFACTORYSHM_H_ */
