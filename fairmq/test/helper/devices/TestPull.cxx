/********************************************************************************
 * Copyright (C) 2015-2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * @file fairmq/test/helper/devices/TestPull.cxx
 */

#include <FairMQDevice.h>
#include <FairMQLogger.h>

namespace fair
{
namespace mq
{
namespace test
{

using namespace std;

class Pull : public FairMQDevice
{
  protected:
    auto Run() -> void override
    {
        auto msg = FairMQMessagePtr{NewMessage()};

        if (Receive(msg, "data") >= 0)
        {
            LOG(INFO) << "PUSH-PULL test successfull";
        }
    };
};

} // namespace test
} // namespace mq
} // namespace fair
