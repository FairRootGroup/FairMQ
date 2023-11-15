/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <fairmq/Device.h>
#include <fairmq/runDevice.h>
#include <fairmq/tools/RateLimit.h>

#include <cstdint>
#include <mutex>
#include <thread>

namespace bpo = boost::program_options;

struct Sampler : fair::mq::Device
{
    void InitTask() override
    {
        fExternalRegion = fConfig->GetProperty<bool>("external-region");
        fMsgSize = fConfig->GetProperty<int>("msg-size");
        fLinger = fConfig->GetProperty<uint32_t>("region-linger");
        fMaxIterations = fConfig->GetProperty<uint64_t>("max-iterations");
        fChanName = fConfig->GetProperty<std::string>("chan-name");
        fSamplingRate = fConfig->GetProperty<float>("sampling-rate");
        fRCSegmentSize = fConfig->GetProperty<uint64_t>("rc-segment-size");

        GetChannel(fChanName, 0).Transport()->SubscribeToRegionEvents([](fair::mq::RegionInfo info) {
            LOG(info) << "Region event: " << info.event << ": "
                    << (info.managed ? "managed" : "unmanaged")
                    << ", id: " << info.id
                    << ", ptr: " << info.ptr
                    << ", size: " << info.size
                    << ", flags: " << info.flags;
        });

        fair::mq::RegionConfig regionCfg;
        regionCfg.linger = fLinger; // delay in ms before region destruction to collect outstanding events
        // options for testing with an externally-created -region
        if (fExternalRegion) {
            regionCfg.id = 1;
            regionCfg.removeOnDestruction = false;
        }
        regionCfg.lock = !fExternalRegion; // mlock region after creation
        regionCfg.zero = !fExternalRegion; // zero region content after creation
        regionCfg.rcSegmentSize = fRCSegmentSize; // size of the corresponding reference count segment
        fRegion = fair::mq::UnmanagedRegionPtr(NewUnmanagedRegionFor(
            fChanName, // region is created using the transport of this channel...
            0,      // ... and this sub-channel
            10000000, // region size
            [this](const std::vector<fair::mq::RegionBlock>& blocks) { // callback to be called when message buffers no longer needed by transport
                std::lock_guard<std::mutex> lock(fMtx);
                fNumUnackedMsgs -= blocks.size();
                if (fMaxIterations > 0) {
                    LOG(info) << "Received " << blocks.size() << " acks";
                }
            },
            regionCfg
        ));
    }

    void Run() override
    {

        fair::mq::tools::RateLimiter rateLimiter(fSamplingRate);

        while (!NewStatePending()) {
            fair::mq::Parts parts;
            // make 64 parts
            for (int i = 0; i < 64; ++i) {
                parts.AddPart(NewMessageFor(
                    fChanName, // channel
                    0, // sub-channel
                    fRegion, // region
                    fRegion->GetData(), // ptr within region
                    fMsgSize, // offset from ptr
                    nullptr // hint
                    ));
            }

            std::lock_guard<std::mutex> lock(fMtx);
            fNumUnackedMsgs += parts.Size();
            if (Send(parts, fChanName, 0) > 0) {
                if (fMaxIterations > 0 && ++fNumIterations >= fMaxIterations) {
                    LOG(info) << "Configured maximum number of iterations reached. Stopping sending.";
                    break;
                }
                if (fSamplingRate > 0.001) {
                    rateLimiter.maybe_sleep();
                }
            }
        }

        // wait for all acks to arrive
        while (!NewStatePending()) {
            {
                std::lock_guard<std::mutex> lock(fMtx);
                if (fNumUnackedMsgs == 0) {
                    break;
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(25));
        }

        if (fNumUnackedMsgs != 0) {
            LOG(info) << "Done, still not acknowledged: " << fNumUnackedMsgs;
        } else {
            LOG(info) << "All acknowledgements received.";
        }
    }

    void ResetTask() override
    {
        fRegion.reset();
        GetChannel(fChanName, 0).Transport()->UnsubscribeFromRegionEvents();
    }

  private:
    int fExternalRegion = false;
    int fMsgSize = 10000;
    uint32_t fLinger = 100;
    uint64_t fMaxIterations = 0;
    uint64_t fNumIterations = 0;
    uint64_t fRCSegmentSize = 10000000;
    fair::mq::UnmanagedRegionPtr fRegion = nullptr;
    std::mutex fMtx;
    uint64_t fNumUnackedMsgs = 0;
    std::string fChanName;
    float fSamplingRate = 0.;
};

void addCustomOptions(bpo::options_description& options)
{
    options.add_options()
        ("chan-name", bpo::value<std::string>()->default_value("data"), "name of the output channel")
        ("msg-size", bpo::value<int>()->default_value(1000), "Message size in bytes")
        ("sampling-rate", bpo::value<float>()->default_value(0.), "Sampling rate (Hz).")
        ("region-linger", bpo::value<uint32_t>()->default_value(100), "Linger period for regions")
        ("max-iterations", bpo::value<uint64_t>()->default_value(0), "Maximum number of iterations of Run/ConditionalRun/OnData (0 - infinite)")
        ("external-region", bpo::value<bool>()->default_value(false), "Use region created by another process")
        ("rc-segment-size", bpo::value<uint64_t>()->default_value(10000000), "Size of the reference count segment for Unamanged Region");
}

std::unique_ptr<fair::mq::Device> getDevice(fair::mq::ProgOptions& /*config*/)
{
    return std::make_unique<Sampler>();
}
