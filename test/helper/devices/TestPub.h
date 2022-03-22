/********************************************************************************
 * Copyright (C) 2015-2022 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_TEST_PUB_H
#define FAIR_MQ_TEST_PUB_H

#include <fairmq/Device.h>
#include <fairlogger/Logger.h>
#include <chrono>
#include <thread>

namespace fair::mq::test
{

class Pub : public Device
{
  protected:
    auto InitTask() -> void override
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    auto Run() -> void override
    {
        auto ready1 = NewMessage();
        auto ready2 = NewMessage();
        auto r1 = Receive(ready1, "control");
        auto r2 = Receive(ready2, "control");
        if (r1 >= 0 && r2 >= 0)
        {
            LOG(info) << "Received both ready signals, proceeding to publish data";

            auto msg = NewMessage();
            auto d1 = Send(msg, "data");
            if (d1 >= 0)
            {
                LOG(info) << "Sent data: d1 = " << d1;
            }
            else
            {
                LOG(error) << "Failed sending data: d1 = " << d1;
            }

            auto ack1 = NewMessage();
            auto ack2 = NewMessage();
            auto a1 = Receive(ack1, "control");
            auto a2 = Receive(ack2, "control");
            if (a1 >= 0 && a2 >= 0)
            {
                LOG(info) << "PUB-SUB test successfull";
            }
            else
            {
                LOG(error) << "Failed receiving ack signal: a1 = " << a1 << ", a2 = " << a2;
            }
        }
        else
        {
            LOG(error) << "Failed receiving ready signal: r1 = " << r1 << ", r2 = " << r2;
        }
    }
};

} // namespace fair::mq::test

#endif /* FAIR_MQ_TEST_PUB_H */
