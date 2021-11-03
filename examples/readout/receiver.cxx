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

using namespace std;
using namespace fair::mq;
namespace bpo = boost::program_options;

namespace {

struct Receiver : Device
{
    void InitTask() override
    {
        // Get the fMaxIterations value from the command line options (via fConfig)
        fMaxIterations = fConfig->GetProperty<uint64_t>("max-iterations");
    }

    void Run() override
    {
        Channel& dataInChannel = GetChannel("sr", 0);

        while (!NewStatePending()) {
            auto msg(dataInChannel.NewMessage());
            dataInChannel.Receive(msg);

            if (fMaxIterations > 0 && ++fNumIterations >= fMaxIterations) {
                LOG(info) << "Configured max number of iterations reached. Leaving RUNNING state.";
                break;
            }
        }
    }

  private:
    uint64_t fMaxIterations = 0;
    uint64_t fNumIterations = 0;
};

}   // namespace

void addCustomOptions(bpo::options_description& options)
{
    options.add_options()(
        "max-iterations",
        bpo::value<uint64_t>()->default_value(0),
        "Maximum number of iterations of Run/ConditionalRun/OnData (0 - infinite)");
}

unique_ptr<Device> getDevice(ProgOptions& /*config*/) { return make_unique<Receiver>(); }
