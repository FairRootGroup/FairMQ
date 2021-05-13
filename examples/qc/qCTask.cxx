/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <fairmq/Device.h>
#include <fairmq/runDevice.h>

class QCTask : public FairMQDevice
{
  public:
    QCTask()
    {
        OnData("qc", [](FairMQMessagePtr& /*msg*/, int) {
            LOG(info) << "received data";
            return false;
        });
    }
};

namespace bpo = boost::program_options;
void addCustomOptions(bpo::options_description& /*options*/) {}
std::unique_ptr<fair::mq::Device> getDevice(fair::mq::ProgOptions& /*config*/)
{
    return std::make_unique<QCTask>();
}
