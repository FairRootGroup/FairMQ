/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <fairmq/Device.h>
#include <fairmq/runDevice.h>

#include <thread>

namespace bpo = boost::program_options;

class Sampler1 : public FairMQDevice
{
  public:
    Sampler1()
        : fAckListener()
        , fMaxIterations(0)
        , fNumIterations(0)
    {}

  protected:
    void InitTask() override
    {
        fMaxIterations = fConfig->GetProperty<uint64_t>("max-iterations");
    }

    void PreRun() override
    {
        fAckListener = std::thread(&Sampler1::ListenForAcks, this);
    }

    bool ConditionalRun() override
    {
        // Creates a message using the transport of channel data1
        FairMQMessagePtr msg(NewMessageFor("data1", 0, 1000000));

        // in case of error or transfer interruption, return false to go to IDLE state
        // successfull transfer will return number of bytes transfered (can be 0 if sending an empty message).
        if (Send(msg, "data1") < 0) {
            return false;
        }

        if (fMaxIterations > 0 && ++fNumIterations >= fMaxIterations) {
            LOG(info) << "Configured maximum number of iterations reached. Leaving RUNNING state.";
            return false;
        }

        return true;
    }

    void PostRun() override
    {
        fAckListener.join();
    }

    void ListenForAcks()
    {
        uint64_t numAcks = 0;

        while (!NewStatePending()) {
            FairMQMessagePtr ack(NewMessageFor("ack", 0));
            if (Receive(ack, "ack") < 0) {
                break;
            }
            ++numAcks;
        }

        LOG(info) << "Acknowledged " << numAcks << " messages";
    }

    std::thread fAckListener;
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
    return std::make_unique<Sampler1>();
}
