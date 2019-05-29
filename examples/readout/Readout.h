/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
#ifndef FAIRMQEXAMPLEREADOUTREADOUT_H
#define FAIRMQEXAMPLEREADOUTREADOUT_H

#include <atomic>
#include <thread>
#include <chrono>

#include "FairMQDevice.h"

namespace example_readout
{

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
                                                                [this](void* /*data*/, size_t /*size*/, void* /*hint*/) { // callback to be called when message buffers no longer needed by transport
                                                                    --fNumUnackedMsgs;
                                                                    if (fMaxIterations > 0) {
                                                                        LOG(debug) << "Received ack";
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

} // namespace example_readout

#endif /* FAIRMQEXAMPLEREADOUTREADOUT_H */
