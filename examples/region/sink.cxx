/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <fairmq/Device.h>
#include <fairmq/runDevice.h>

namespace bpo = boost::program_options;

struct Sink : fair::mq::Device
{
    void InitTask() override
    {
        // Get the fMaxIterations value from the command line options (via fConfig)
        fMaxIterations = fConfig->GetProperty<uint64_t>("max-iterations");
        fChannels.at("data").at(0).Transport()->SubscribeToRegionEvents([](FairMQRegionInfo info) {
            LOG(info) << "Region event: " << info.event << ": "
                    << (info.managed ? "managed" : "unmanaged")
                    << ", id: " << info.id
                    << ", ptr: " << info.ptr
                    << ", size: " << info.size
                    << ", flags: " << info.flags;
        });
    }
    void Run() override
    {
        FairMQChannel& dataInChannel = fChannels.at("data").at(0);

        while (!NewStatePending()) {
            FairMQMessagePtr msg(dataInChannel.Transport()->CreateMessage());
            dataInChannel.Receive(msg);

            // void* ptr = msg->GetData();
            // char* cptr = static_cast<char*>(ptr);
            // LOG(info) << "check: " << cptr[3];

            if (fMaxIterations > 0 && ++fNumIterations >= fMaxIterations) {
                LOG(info) << "Configured maximum number of iterations reached. Leaving RUNNING state.";
                break;
            }
        }
    }
    void ResetTask() override
    {
        fChannels.at("data").at(0).Transport()->UnsubscribeFromRegionEvents();
    }

  private:
    uint64_t fMaxIterations = 0;
    uint64_t fNumIterations = 0;
};


void addCustomOptions(bpo::options_description& options)
{
    options.add_options()
        ("max-iterations", bpo::value<uint64_t>()->default_value(0), "Maximum number of iterations of Run/ConditionalRun/OnData (0 - infinite)");
}

std::unique_ptr<fair::mq::Device> getDevice(fair::mq::ProgOptions& /*config*/)
{
    return std::make_unique<Sink>();
}
