/********************************************************************************
 * Copyright (C) 2015-2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_TEST_REQ_H
#define FAIR_MQ_TEST_REQ_H

#include <FairMQDevice.h>
#include <FairMQLogger.h>
#include <thread>

namespace fair
{
namespace mq
{
namespace test
{

class Req : public FairMQDevice
{
  protected:
    auto InitTask() -> void override
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    auto Run() -> void override
    {
        auto request = FairMQMessagePtr{NewMessage()};
        Send(request, "data");

        auto reply = FairMQMessagePtr{NewMessage()};
        if (Receive(reply, "data") >= 0)
        {
            LOG(info) << "received reply";
        }
    };
};

} // namespace test
} // namespace mq
} // namespace fair

#endif /* FAIR_MQ_TEST_REQ_H */
