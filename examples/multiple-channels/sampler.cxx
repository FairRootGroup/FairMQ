/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <fairmq/Device.h>
#include <fairmq/Poller.h>
#include <fairmq/runDevice.h>

#include <chrono>
#include <string>
#include <thread> // this_thread::sleep_for

namespace bpo = boost::program_options;

struct Sampler : fair::mq::Device
{
    void InitTask() override
    {
        fText = fConfig->GetProperty<std::string>("text");
        fMaxIterations = fConfig->GetProperty<uint64_t>("max-iterations");
    }

    void Run() override
    {
        fair::mq::PollerPtr poller(NewPoller("data", "broadcast"));

        while (!NewStatePending()) {
            poller->Poll(100);

            if (poller->CheckInput("broadcast", 0)) {
                fair::mq::MessagePtr msg(NewMessage());

                if (Receive(msg, "broadcast") > 0) {
                    LOG(info) << "Received broadcast: \"" << std::string(static_cast<char*>(msg->GetData()), msg->GetSize()) << "\"";
                }
            }

            if (poller->CheckOutput("data", 0)) {
                std::this_thread::sleep_for(std::chrono::seconds(1));

                fair::mq::MessagePtr msg(NewSimpleMessage(fText));

                if (Send(msg, "data") > 0) {
                    LOG(info) << "Sent \"" << fText << "\"";

                    if (fMaxIterations > 0 && ++fNumIterations >= fMaxIterations) {
                        LOG(info) << "Configured maximum number of iterations reached. Leaving RUNNING state.";
                        break;
                    }
                }
            }
        }
    }

  private:
    std::string fText;
    uint64_t fMaxIterations = 0;
    uint64_t fNumIterations = 0;
};

void addCustomOptions(bpo::options_description& options)
{
    options.add_options()
        ("text", bpo::value<std::string>()->default_value("Hello"), "Text to send out")
        ("max-iterations", bpo::value<uint64_t>()->default_value(0), "Maximum number of iterations of Run/ConditionalRun/OnData (0 - infinite)");
}

std::unique_ptr<fair::mq::Device> getDevice(fair::mq::ProgOptions& /*config*/)
{
    return std::make_unique<Sampler>();
}
