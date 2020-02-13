/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "runFairMQDevice.h"
#include "FairMQDevice.h"

#include <string>

class Sink : public FairMQDevice
{
  public:
    Sink()
        : fMaxIterations(0)
        , fNumIterations(0)
    {
        OnData("data2", &Sink::HandleData);
    }

  protected:
    virtual void InitTask()
    {
        fMaxIterations = fConfig->GetProperty<uint64_t>("max-iterations");
    }

    bool HandleData(FairMQMessagePtr& /*msg*/, int /*index*/)
    {
        if (fMaxIterations > 0 && ++fNumIterations >= fMaxIterations) {
            LOG(info) << "Configured maximum number of iterations reached. Leaving RUNNING state.";
            return false;
        }

        return true;
    }

  private:
    uint64_t fMaxIterations;
    uint64_t fNumIterations;
};

namespace bpo = boost::program_options;
void addCustomOptions(bpo::options_description& options)
{
    options.add_options()
        ("max-iterations", bpo::value<uint64_t>()->default_value(0), "Maximum number of iterations of Run/ConditionalRun/OnData (0 - infinite)");
}
FairMQDevicePtr getDevice(const fair::mq::ProgOptions& /*config*/) { return new Sink(); }
