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
    Sink()
    {
        // register a handler for data arriving on "data" channel
        OnData("data1", &Sink::HandleData1);
        OnData("data2", &Sink::HandleData2);
    }

    void InitTask() override
    {
        fMaxIterations = fConfig->GetProperty<uint64_t>("max-iterations");
    }

    // handler is called whenever a message arrives on "data", with a reference to the message and a sub-channel index (here 0)
    bool HandleData1(FairMQMessagePtr& /*msg*/, int /*index*/)
    {
        fNumIterations1++;
        // Creates a message using the transport of channel ack
        FairMQMessagePtr ack(NewMessageFor("ack", 0));
        if (Send(ack, "ack") < 0) {
            return false;
        }

        // return true if want to be called again (otherwise go to IDLE state)
        return CheckIterations();
    }

    // handler is called whenever a message arrives on "data", with a reference to the message and a sub-channel index (here 0)
    bool HandleData2(FairMQMessagePtr& /*msg*/, int /*index*/)
    {
        fNumIterations2++;
        // return true if want to be called again (otherwise go to IDLE state)
        return CheckIterations();
    }

    bool CheckIterations()
    {
        if (fMaxIterations > 0) {
            if (fNumIterations1 >= fMaxIterations && fNumIterations2 >= fMaxIterations) {
                LOG(info) << "Configured maximum number of iterations reached & Received messages from both sources. Leaving RUNNING state.";
                return false;
            }
        }

        return true;
    }

  private:
    uint64_t fMaxIterations = 0;
    uint64_t fNumIterations1 = 0;
    uint64_t fNumIterations2 = 0;
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
