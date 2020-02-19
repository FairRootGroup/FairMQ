/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "runFairMQDevice.h"
#include "FairMQDevice.h"

#include <thread> // this_thread::sleep_for
#include <chrono>

class Sampler : public FairMQDevice
{
  public:
    Sampler()
        : fMaxIterations(0)
        , fNumIterations(0)
    {}

  protected:
    uint64_t fMaxIterations;
    uint64_t fNumIterations;

    virtual void InitTask()
    {
        fMaxIterations = fConfig->GetProperty<uint64_t>("max-iterations");
    }

    virtual bool ConditionalRun()
    {
        FairMQMessagePtr msg(NewMessage(1000));

        if (Send(msg, "data1") < 0) {
            return false;
        } else if (fMaxIterations > 0 && ++fNumIterations >= fMaxIterations) {
            LOG(info) << "Configured maximum number of iterations reached. Leaving RUNNING state.";
            return false;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        return true;
    }
};

namespace bpo = boost::program_options;
void addCustomOptions(bpo::options_description& options)
{
    options.add_options()
        ("max-iterations", bpo::value<uint64_t>()->default_value(0), "Maximum number of iterations of Run/ConditionalRun/OnData (0 - infinite)");
}
FairMQDevicePtr getDevice(const fair::mq::ProgOptions& /*config*/) { return new Sampler(); }
