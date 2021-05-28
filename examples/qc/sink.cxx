/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <fairmq/Device.h>
#include <fairmq/runDevice.h>

#include <string>

struct Sink : fair::mq::Device
{
    Sink() { OnData("data2", &Sink::HandleData); }
    bool HandleData(FairMQMessagePtr& /*msg*/, int /*index*/) { return true; }
};

namespace bpo = boost::program_options;
void addCustomOptions(bpo::options_description&) {}
std::unique_ptr<fair::mq::Device> getDevice(fair::mq::ProgOptions& /*config*/)
{
  return std::make_unique<Sink>();
}
