/********************************************************************************
 * Copyright (C) 2015-2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_TEST_REP_H
#define FAIR_MQ_TEST_REP_H

#include <FairMQDevice.h>
#include <FairMQLogger.h>
#include <thread>

namespace fair::mq::test
{

class Rep : public FairMQDevice
{
  protected:
    auto InitTask() -> void override
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    auto Run() -> void override
    {
        auto request1 = FairMQMessagePtr{NewMessage()};
        if (Receive(request1, "data") >= 0)
        {
            LOG(info) << "Received request 1";
            auto reply = FairMQMessagePtr{NewMessage()};
            Send(reply, "data");
        }
        auto request2 = FairMQMessagePtr{NewMessage()};
        if (Receive(request2, "data") >= 0)
        {
            LOG(info) << "Received request 2";
            auto reply = FairMQMessagePtr{NewMessage()};
            Send(reply, "data");
        }

        LOG(info) << "REQ-REP test successfull";
    };
};

} // namespace fair::mq::test

#endif /* FAIR_MQ_TEST_REP_H */
