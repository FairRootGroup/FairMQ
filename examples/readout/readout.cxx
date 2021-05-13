/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <fairmq/Device.h>
#include <fairmq/runDevice.h>

#include <atomic>
#include <thread>
#include <chrono>

namespace bpo = boost::program_options;

class Readout : public FairMQDevice
{
  public:
    Readout()
        : fMsgSize(10000)
        , fMaxIterations(0)
        , fNumIterations(0)
        , fRegion(nullptr)
        , fNumUnackedMsgs(0)
    {}

  protected:
    void InitTask() override
    {
        fMsgSize = fConfig->GetProperty<int>("msg-size");
        fMaxIterations = fConfig->GetProperty<uint64_t>("max-iterations");

        fRegion = FairMQUnmanagedRegionPtr(NewUnmanagedRegionFor("rb",
                                                                0,
                                                                10000000,
                                                                [this](const std::vector<fair::mq::RegionBlock>& blocks) { // callback to be called when message buffers no longer needed by transport
                                                                    fNumUnackedMsgs -= blocks.size();
                                                                    if (fMaxIterations > 0) {
                                                                        LOG(debug) << "Received " << blocks.size() << " acks";
                                                                    }
                                                                }
                                                                ));
    }

    bool ConditionalRun() override
    {
        FairMQMessagePtr msg(NewMessageFor("rb", // channel
                                            0, // sub-channel
                                            fRegion, // region
                                            fRegion->GetData(), // ptr within region
                                            fMsgSize, // offset from ptr
                                            nullptr // hint
                                            ));

        if (Send(msg, "rb", 0) > 0) {
            ++fNumUnackedMsgs;

            if (fMaxIterations > 0 && ++fNumIterations >= fMaxIterations) {
                LOG(info) << "Configured maximum number of iterations reached. Leaving RUNNING state.";
                return false;
            }
        }

        return true;
    }
    void ResetTask() override
    {
        // if not all messages acknowledged, wait for a bit. But only once, since receiver could be already dead.
        if (fNumUnackedMsgs != 0) {
            LOG(debug) << "waiting for all acknowledgements... (" << fNumUnackedMsgs << ")";
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            LOG(debug) << "done, still unacked: " << fNumUnackedMsgs;
        }
        fRegion.reset();
    }

  private:
    int fMsgSize;
    uint64_t fMaxIterations;
    uint64_t fNumIterations;
    FairMQUnmanagedRegionPtr fRegion;
    std::atomic<uint64_t> fNumUnackedMsgs;
};

void addCustomOptions(bpo::options_description& options)
{
    options.add_options()
        ("msg-size", bpo::value<int>()->default_value(1000), "Message size in bytes")
        ("max-iterations", bpo::value<uint64_t>()->default_value(0), "Maximum number of iterations of Run/ConditionalRun/OnData (0 - infinite)");
}

std::unique_ptr<fair::mq::Device> getDevice(fair::mq::ProgOptions& /*config*/)
{
    return std::make_unique<Readout>();
}
