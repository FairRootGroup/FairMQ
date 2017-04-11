/********************************************************************************
 * Copyright (C) 2015-2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * @file fairmq/test/helper/devices/TestPub.cxx
 */

#include <FairMQDevice.h>
#include <FairMQLogger.h>

namespace fair
{
namespace mq
{
namespace test
{

class Pub : public FairMQDevice
{
  protected:
    auto Run() -> void override
    {
        auto ready1 = FairMQMessagePtr{NewMessage()};
        auto ready2 = FairMQMessagePtr{NewMessage()};
        auto r1 = Receive(ready1, "control");
        auto r2 = Receive(ready2, "control");
        if (r1 >= 0 && r2 >= 0)
        {
            LOG(INFO) << "Received both ready signals, proceeding to publish data";

            auto msg = FairMQMessagePtr{NewMessage()};
            auto d1 = Send(msg, "data");
            if (d1 >= 0)
            {
                LOG(INFO) << "Sent data: d1 = " << d1;
            }
            else
            {
                LOG(ERROR) << "Failed sending data: d1 = " << d1;
            }

            auto ack1 = FairMQMessagePtr{NewMessage()};
            auto ack2 = FairMQMessagePtr{NewMessage()};
            auto a1 = Receive(ack1, "control");
            auto a2 = Receive(ack2, "control");
            if (a1 >= 0 && a2 >= 0)
            {
                LOG(INFO) << "PUB-SUB test successfull";
            }
            else
            {
                LOG(ERROR) << "Failed receiving ack signal: a1 = " << a1 << ", a2 = " << a2;
            }
        }
        else
        {
            LOG(ERROR) << "Failed receiving ready signal: r1 = " << r1 << ", r2 = " << r2;
        }
    };
};

} // namespace test
} // namespace mq
} // namespace fair
