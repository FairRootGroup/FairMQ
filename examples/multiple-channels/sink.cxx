/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <fairmq/Device.h>
#include <fairmq/runDevice.h>

#include <cstdint> // uint64_t
#include <string>

namespace bpo = boost::program_options;

class Sink : public FairMQDevice
{
  public:
    Sink()
        : fReceivedData(false)
        , fReceivedBroadcast(false)
        , fMaxIterations(0)
        , fNumIterations(0)
    {
        OnData("broadcast", &Sink::HandleBroadcast);
        OnData("data", &Sink::HandleData);
    }

    void InitTask() override
    {
        fMaxIterations = fConfig->GetProperty<uint64_t>("max-iterations");
    }

    bool HandleBroadcast(FairMQMessagePtr& msg, int /*index*/)
    {
        LOG(info) << "Received broadcast: \"" << std::string(static_cast<char*>(msg->GetData()), msg->GetSize()) << "\"";
        fReceivedBroadcast = true;

        return CheckIterations();
    }

    bool HandleData(FairMQMessagePtr& msg, int /*index*/)
    {
        LOG(info) << "Received message: \"" << std::string(static_cast<char*>(msg->GetData()), msg->GetSize()) << "\"";
        fReceivedData = true;

        return CheckIterations();
    }

    bool CheckIterations()
    {
        if (fMaxIterations > 0) {
            if (fReceivedData && fReceivedBroadcast && ++fNumIterations >= fMaxIterations) {
                LOG(info) << "Configured maximum number of iterations reached & Received messages from both sources. Leaving RUNNING state.";
                return false;
            }
        }

        return true;
    }

  private:
    bool fReceivedData;
    bool fReceivedBroadcast;
    uint64_t fMaxIterations;
    uint64_t fNumIterations;
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
