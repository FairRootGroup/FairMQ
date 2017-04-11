/********************************************************************************
 * Copyright (C) 2015-2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * @file fairmq/test/helper/devices/TestSub.cxx
 */

#include <FairMQDevice.h>
#include <FairMQLogger.h>

namespace fair
{
namespace mq
{
namespace test
{

class Sub : public FairMQDevice
{
  protected:
    auto Run() -> void override
    {
        auto ready = FairMQMessagePtr{NewMessage()};
        auto r1 = Send(ready, "control");
        if (r1 >= 0)
        {
            LOG(INFO) << "Sent first control signal";
            auto msg = FairMQMessagePtr{NewMessage()};
            auto d1 = Receive(msg, "data");
            if (d1 >= 0)
            {
                LOG(INFO) << "Received data";
                auto ack = FairMQMessagePtr{NewMessage()};
                auto a1 = Send(ack, "control");
                if (a1 >= 0)
                {
                    LOG(INFO) << "Sent second control signal";
                }
                else
                {
                    LOG(ERROR) << "Failed sending ack signal: a1 = " << a1;
                }
            }
            else
            {
                LOG(ERROR) << "Failed receiving data: d1 = " << d1;
            }
        }
        else
        {
            LOG(ERROR) << "Failed sending ready signal: r1 = " << r1;
        }
    };
};

} // namespace test
} // namespace mq
} // namespace fair
