/********************************************************************************
 * Copyright (C) 2016-2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIRMQTRANSPORTFACTORYSHM_H_
#define FAIRMQTRANSPORTFACTORYSHM_H_

#include <fairmq/shmem/Manager.h>
#include <fairmq/shmem/Common.h>

#include "FairMQTransportFactory.h"
#include "FairMQMessageSHM.h"
#include "FairMQSocketSHM.h"
#include "FairMQPollerSHM.h"
#include "FairMQUnmanagedRegionSHM.h"
#include <options/FairMQProgOptions.h>

#include <boost/interprocess/sync/named_mutex.hpp>

#include <vector>
#include <string>
#include <thread>
#include <atomic>

class FairMQTransportFactorySHM final : public FairMQTransportFactory
{
  public:
    FairMQTransportFactorySHM(const std::string& id = "", const FairMQProgOptions* config = nullptr);
    FairMQTransportFactorySHM(const FairMQTransportFactorySHM&) = delete;
    FairMQTransportFactorySHM operator=(const FairMQTransportFactorySHM&) = delete;

    FairMQMessagePtr CreateMessage() const override;
    FairMQMessagePtr CreateMessage(const size_t size) const override;
    FairMQMessagePtr CreateMessage(void* data, const size_t size, fairmq_free_fn* ffn, void* hint = nullptr) const override;
    FairMQMessagePtr CreateMessage(FairMQUnmanagedRegionPtr& region, void* data, const size_t size, void* hint = 0) const override;

    FairMQSocketPtr CreateSocket(const std::string& type, const std::string& name) const override;

    FairMQPollerPtr CreatePoller(const std::vector<FairMQChannel>& channels) const override;
    FairMQPollerPtr CreatePoller(const std::vector<const FairMQChannel*>& channels) const override;
    FairMQPollerPtr CreatePoller(const std::unordered_map<std::string, std::vector<FairMQChannel>>& channelsMap, const std::vector<std::string>& channelList) const override;

    FairMQUnmanagedRegionPtr CreateUnmanagedRegion(const size_t size, FairMQRegionCallback callback = nullptr) const override;

    fair::mq::Transport GetType() const override;

    void Interrupt() override { FairMQSocketSHM::Interrupt(); }
    void Resume() override { FairMQSocketSHM::Resume(); }
    void Reset() override {}

    ~FairMQTransportFactorySHM() override;

  private:
    void SendHeartbeats();
    void StartMonitor();

    static fair::mq::Transport fTransportType;
    std::string fDeviceId;
    std::string fShmId;
    void* fContext;
    std::thread fHeartbeatThread;
    std::atomic<bool> fSendHeartbeats;
    std::unique_ptr<boost::interprocess::named_mutex> fShMutex;
    fair::mq::shmem::DeviceCounter* fDeviceCounter;
    std::unique_ptr<fair::mq::shmem::Manager> fManager;
};

#endif /* FAIRMQTRANSPORTFACTORYSHM_H_ */
