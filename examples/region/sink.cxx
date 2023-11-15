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

struct Sink : Device
{
    void InitTask() override
    {
        // Get the fMaxIterations value from the command line options (via fConfig)
        fMaxIterations = fConfig->GetProperty<uint64_t>("max-iterations");
        fChanName = fConfig->GetProperty<std::string>("chan-name");
        GetChannel(fChanName, 0).Transport()->SubscribeToRegionEvents([](RegionInfo info) {
            LOG(info) << "Region event: " << info.event << ": "
                      << (info.managed ? "managed" : "unmanaged") << ", id: " << info.id
                      << ", ptr: " << info.ptr << ", size: " << info.size
                      << ", flags: " << info.flags;
        });
    }

    void Run() override
    {
        Channel& dataIn = GetChannel(fChanName, 0);

        while (!NewStatePending()) {
            fair::mq::Parts parts;
            dataIn.Receive(parts);

            if (fMaxIterations > 0 && ++fNumIterations >= fMaxIterations) {
                LOG(info) << "Configured max number of iterations reached. Leaving RUNNING state.";
                break;
            }
        }
    }

    void ResetTask() override
    {
        GetChannel(fChanName, 0).Transport()->UnsubscribeFromRegionEvents();
    }

  private:
    uint64_t fMaxIterations = 0;
    uint64_t fNumIterations = 0;
    std::string fChanName;
};

}   // namespace

void addCustomOptions(bpo::options_description& options)
{
    options.add_options()
        ("chan-name", bpo::value<std::string>()->default_value("data"), "name of the input channel")
        ("max-iterations", bpo::value<uint64_t>()->default_value(0), "Maximum number of iterations of Run/ConditionalRun/OnData (0 - infinite)");
}

unique_ptr<Device> getDevice(ProgOptions& /*config*/) { return make_unique<Sink>(); }
