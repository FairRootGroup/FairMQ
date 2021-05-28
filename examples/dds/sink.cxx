/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <fairmq/Device.h>
#include <fairmq/runDevice.h>

#include <string>

namespace bpo = boost::program_options;

struct Sink : fair::mq::Device
{
    Sink()
    {
        OnData("data2", &Sink::HandleData);
    }

    void InitTask() override
    {
        fIterations = fConfig->GetValue<uint64_t>("iterations");
    }

    bool HandleData(FairMQMessagePtr& msg, int)
    {
        LOG(info) << "Received: \"" << std::string(static_cast<char*>(msg->GetData()), msg->GetSize()) << "\"";

        if (fIterations > 0) {
            ++fCounter;
            if (fCounter >= fIterations) {
                LOG(info) << "Received " << fCounter << " messages. Finished.";
                return false;
            }
        }
        // return true if want to be called again (otherwise go to IDLE state)
        return true;
    }

  private:
    uint64_t fIterations = 0;
    uint64_t fCounter = 0;
};
void addCustomOptions(bpo::options_description& options)
{
    options.add_options()(
        "iterations,i",
        bpo::value<uint64_t>()->default_value(1000),
        "Maximum number of iterations of Run/ConditionalRun/OnData (0 - infinite)");
}

std::unique_ptr<fair::mq::Device> getDevice(fair::mq::ProgOptions& /*config*/)
{
    return std::make_unique<Sink>();
}
