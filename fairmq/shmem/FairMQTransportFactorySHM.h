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
#include <fairmq/ProgOptions.h>

#include <vector>
#include <string>
#include <thread>
#include <atomic>

class FairMQTransportFactorySHM final : public FairMQTransportFactory
{
  public:
    FairMQTransportFactorySHM(const std::string& id = "", const fair::mq::ProgOptions* config = nullptr);
    FairMQTransportFactorySHM(const FairMQTransportFactorySHM&) = delete;
    FairMQTransportFactorySHM operator=(const FairMQTransportFactorySHM&) = delete;

    FairMQMessagePtr CreateMessage() override;
    FairMQMessagePtr CreateMessage(const size_t size) override;
    FairMQMessagePtr CreateMessage(void* data, const size_t size, fairmq_free_fn* ffn, void* hint = nullptr) override;
    FairMQMessagePtr CreateMessage(FairMQUnmanagedRegionPtr& region, void* data, const size_t size, void* hint = 0) override;

    FairMQSocketPtr CreateSocket(const std::string& type, const std::string& name) override;

    FairMQPollerPtr CreatePoller(const std::vector<FairMQChannel>& channels) const override;
    FairMQPollerPtr CreatePoller(const std::vector<FairMQChannel*>& channels) const override;
    FairMQPollerPtr CreatePoller(const std::unordered_map<std::string, std::vector<FairMQChannel>>& channelsMap, const std::vector<std::string>& channelList) const override;

    FairMQUnmanagedRegionPtr CreateUnmanagedRegion(const size_t size, FairMQRegionCallback callback = nullptr, const std::string& path = "", int flags = 0) const override;

    fair::mq::Transport GetType() const override;

    void Interrupt() override { FairMQSocketSHM::Interrupt(); }
    void Resume() override { FairMQSocketSHM::Resume(); }
    void Reset() override {}

    ~FairMQTransportFactorySHM() override;

  private:
    void SendHeartbeats();

    static fair::mq::Transport fTransportType;
    std::string fDeviceId;
    std::string fShmId;
    void* fZMQContext;
    std::unique_ptr<fair::mq::shmem::Manager> fManager;
    std::thread fHeartbeatThread;
    std::atomic<bool> fSendHeartbeats;
};

#endif /* FAIRMQTRANSPORTFACTORYSHM_H_ */
