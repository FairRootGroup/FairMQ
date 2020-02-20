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
    Sampler() {}

  protected:
    virtual bool ConditionalRun()
    {
        FairMQMessagePtr msg(NewMessage(1000));

        if (Send(msg, "data1") < 0) {
            return false;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        return true;
    }
};

namespace bpo = boost::program_options;
void addCustomOptions(bpo::options_description& options) {}
FairMQDevicePtr getDevice(const fair::mq::ProgOptions& /*config*/) { return new Sampler(); }
