/********************************************************************************
 * Copyright (C) 2014-2021 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <fairmq/Device.h>
#include <fairmq/runDevice.h>
#include <memory>

namespace bpo = boost::program_options;
using namespace std;
using namespace fair::mq;

namespace {

struct Processor : Device
{
    void InitTask() override
    {
        fMaxIterations = fConfig->GetProperty<uint64_t>("max-iterations");
        GetChannel("data1", 0).Transport()->SubscribeToRegionEvents([](RegionInfo info) {
            LOG(info) << "Region event: " << info.event << ": "
                      << (info.managed ? "managed" : "unmanaged") << ", id: " << info.id
                      << ", ptr: " << info.ptr << ", size: " << info.size
                      << ", flags: " << info.flags;
        });
    }

    void Run() override
    {
        Channel& dataIn = GetChannel("data1", 0);
        Channel& dataOut1 = GetChannel("data2", 0);
        Channel& dataOut2 = GetChannel("data3", 0);

        while (!NewStatePending()) {
            fair::mq::Parts inParts;
            dataIn.Receive(inParts);

            fair::mq::Parts outParts1;
            fair::mq::Parts outParts2;

            for (const auto& inPart : inParts) {
                outParts1.AddPart(NewMessage());
                outParts1.fParts.back()->Copy(*inPart);

                outParts2.AddPart(NewMessage());
                outParts2.fParts.back()->Copy(*inPart);
            }

            dataOut1.Send(outParts1);
            dataOut2.Send(outParts2);

            if (fMaxIterations > 0 && ++fNumIterations >= fMaxIterations) {
                LOG(info) << "Configured max number of iterations reached. Leaving RUNNING state.";
                break;
            }
        }
    }

    void ResetTask() override
    {
        GetChannel("data1", 0).Transport()->UnsubscribeFromRegionEvents();
    }

  private:
    uint64_t fMaxIterations = 0;
    uint64_t fNumIterations = 0;
};

}   // namespace

void addCustomOptions(bpo::options_description& options)
{
    options.add_options()("max-iterations", bpo::value<uint64_t>()->default_value(0), "Maximum number of iterations of Run/ConditionalRun/OnData (0 - infinite)");
}

unique_ptr<Device> getDevice(ProgOptions& /*config*/) { return make_unique<Processor>(); }
