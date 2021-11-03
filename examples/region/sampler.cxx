/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <fairmq/Device.h>
#include <fairmq/runDevice.h>

#include <cstdint>
#include <mutex>
#include <thread>

namespace bpo = boost::program_options;

struct Sampler : fair::mq::Device
{
    void InitTask() override
    {
        fMsgSize = fConfig->GetProperty<int>("msg-size");
        fLinger = fConfig->GetProperty<uint32_t>("region-linger");
        fMaxIterations = fConfig->GetProperty<uint64_t>("max-iterations");

        GetChannel("data", 0).Transport()->SubscribeToRegionEvents([](FairMQRegionInfo info) {
            LOG(info) << "Region event: " << info.event << ": "
                    << (info.managed ? "managed" : "unmanaged")
                    << ", id: " << info.id
                    << ", ptr: " << info.ptr
                    << ", size: " << info.size
                    << ", flags: " << info.flags;
        });

        fRegion = FairMQUnmanagedRegionPtr(NewUnmanagedRegionFor("data", // region is created using the transport of this channel...
                                                                0,      // ... and this sub-channel
                                                                10000000, // region size
                                                                [this](const std::vector<fair::mq::RegionBlock>& blocks) { // callback to be called when message buffers no longer needed by transport
                                                                    std::lock_guard<std::mutex> lock(fMtx);
                                                                    fNumUnackedMsgs -= blocks.size();

                                                                    if (fMaxIterations > 0) {
                                                                        LOG(info) << "Received " << blocks.size() << " acks";
                                                                    }
                                                                },
                                                                "", // path, if a region is backed by a file
                                                                0,  // flags that are passed for region creation
                                                                fair::mq::RegionConfig{true, true} // additional config: { call mlock on the region, zero the region memory }
                                                                ));
        fRegion->SetLinger(fLinger);
    }

    bool ConditionalRun() override
    {
        FairMQMessagePtr msg(NewMessageFor("data", // channel
                                            0, // sub-channel
                                            fRegion, // region
                                            fRegion->GetData(), // ptr within region
                                            fMsgSize, // offset from ptr
                                            nullptr // hint
                                            ));

        // static_cast<char*>(fRegion->GetData())[3] = 97;
        // LOG(info) << "check: " << static_cast<char*>(fRegion->GetData())[3];
        // std::this_thread::sleep_for(std::chrono::seconds(1));

        std::lock_guard<std::mutex> lock(fMtx);
        ++fNumUnackedMsgs;
        if (Send(msg, "data", 0) > 0) {
            if (fMaxIterations > 0 && ++fNumIterations >= fMaxIterations) {
                LOG(info) << "Configured maximum number of iterations reached. Leaving RUNNING state.";
                return false;
            }
        }

        return true;
    }

    void ResetTask() override
    {
        fRegion.reset();
        {
            std::lock_guard<std::mutex> lock(fMtx);
            if (fNumUnackedMsgs != 0) {
                LOG(info) << "Done, still not acknowledged: " << fNumUnackedMsgs;
            } else {
                LOG(info) << "All acknowledgements received.";
            }
        }
        GetChannel("data", 0).Transport()->UnsubscribeFromRegionEvents();
    }

  private:
    int fMsgSize = 10000;
    uint32_t fLinger = 100;
    uint64_t fMaxIterations = 0;
    uint64_t fNumIterations = 0;
    FairMQUnmanagedRegionPtr fRegion = nullptr;
    std::mutex fMtx;
    uint64_t fNumUnackedMsgs = 0;
};

void addCustomOptions(bpo::options_description& options)
{
    options.add_options()
        ("msg-size", bpo::value<int>()->default_value(1000), "Message size in bytes")
        ("region-linger", bpo::value<uint32_t>()->default_value(100), "Linger period for regions")
        ("max-iterations", bpo::value<uint64_t>()->default_value(0), "Maximum number of iterations of Run/ConditionalRun/OnData (0 - infinite)");
}

std::unique_ptr<fair::mq::Device> getDevice(fair::mq::ProgOptions& /*config*/)
{
    return std::make_unique<Sampler>();
}
