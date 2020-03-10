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
    Sink() { OnData("data2", &Sink::HandleData); }

  protected:
    bool HandleData(FairMQMessagePtr& /*msg*/, int /*index*/) { return true; }
};

namespace bpo = boost::program_options;
void addCustomOptions(bpo::options_description&) {}
FairMQDevicePtr getDevice(const fair::mq::ProgOptions& /*config*/) { return new Sink(); }
