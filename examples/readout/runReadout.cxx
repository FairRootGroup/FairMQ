/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "runFairMQDevice.h"
#include "Readout.h"

namespace bpo = boost::program_options;

void addCustomOptions(bpo::options_description& options)
{
    options.add_options()
        ("msg-size", bpo::value<int>()->default_value(1000), "Message size in bytes")
        ("max-iterations", bpo::value<uint64_t>()->default_value(0), "Maximum number of iterations of Run/ConditionalRun/OnData (0 - infinite)");
}

FairMQDevicePtr getDevice(const fair::mq::ProgOptions& /*config*/)
{
    return new example_readout::Readout();
}
