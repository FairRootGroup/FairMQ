/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "runFairMQDevice.h"
#include "FairMQDevice.h"

class QCConsumer : public FairMQDevice
{
  public:
    QCConsumer()
    {
        OnData("qc", [](FairMQMessagePtr& /*msg*/, int) {
            LOG(info) << "received data";
            return false;
        });
    }
};

namespace bpo = boost::program_options;
void addCustomOptions(bpo::options_description& /*options*/) {}
FairMQDevicePtr getDevice(const fair::mq::ProgOptions& /*config*/) { return new QCConsumer(); }
