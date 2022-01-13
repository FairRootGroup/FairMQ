/********************************************************************************
 * Copyright (C) 2015-2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_TEST_SUB_H
#define FAIR_MQ_TEST_SUB_H

#include <fairmq/Device.h>
#include <fairlogger/Logger.h>
#include <chrono>
#include <thread>

namespace fair::mq::test
{

class Sub : public FairMQDevice
{
  protected:
    auto InitTask() -> void override
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    auto Run() -> void override
    {
        auto ready = FairMQMessagePtr{NewMessage()};
        auto r1 = Send(ready, "control");
        if (r1 >= 0)
        {
            LOG(info) << "Sent first control signal";
            auto msg = FairMQMessagePtr{NewMessage()};
            auto d1 = Receive(msg, "data");
            if (d1 >= 0)
            {
                LOG(info) << "Received data";
                auto ack = FairMQMessagePtr{NewMessage()};
                auto a1 = Send(ack, "control");
                if (a1 >= 0)
                {
                    LOG(info) << "Sent second control signal";
                }
                else
                {
                    LOG(error) << "Failed sending ack signal: a1 = " << a1;
                }
            }
            else
            {
                LOG(error) << "Failed receiving data: d1 = " << d1;
            }
        }
        else
        {
            LOG(error) << "Failed sending ready signal: r1 = " << r1;
        }
    }
};

} // namespace fair::mq::test

#endif /* FAIR_MQ_TEST_SUB_H */
