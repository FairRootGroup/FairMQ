/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <fairmq/Device.h>
#include <fairmq/runDevice.h>

#include <chrono>
#include <cstdint> // uint64_t
#include <thread> // this_thread::sleep_for

namespace bpo = boost::program_options;

struct Sampler : fair::mq::Device
{
    void InitTask() override
    {
        fNumDataChannels = GetNumSubChannels("data");
        fMaxIterations = fConfig->GetProperty<uint64_t>("max-iterations");
    }

    bool ConditionalRun() override
    {
        // NewSimpleMessage creates a copy of the data and takes care of its destruction (after the transfer takes place).
        // Should only be used for small data because of the cost of an additional copy
        fair::mq::MessagePtr msg(NewSimpleMessage(fCounter++));

        for (int i = 0; i < fNumDataChannels - 1; ++i) {
            fair::mq::MessagePtr msgCopy(NewMessage());
            msgCopy->Copy(*msg);
            Send(msgCopy, "data", i);
        }
        Send(msg, "data", fNumDataChannels - 1);

        if (fMaxIterations > 0 && ++fNumIterations >= fMaxIterations) {
            LOG(info) << "Configured maximum number of iterations reached. Leaving RUNNING state.";
            return false;
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));

        return true;
    }

  private:
    int fNumDataChannels = 0;
    uint64_t fCounter = 0;
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
    return std::make_unique<Sampler>();
}
