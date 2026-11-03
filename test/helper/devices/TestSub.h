/********************************************************************************
 * Copyright (C) 2015-2022 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_TEST_SUB_H
#define FAIR_MQ_TEST_SUB_H

#include "TestDevice.h"

#include <chrono>
#include <thread>

namespace fair::mq::test
{

class Sub : public TestDevice
{
  protected:
    auto Run() -> void override
    {
        // A pub socket drops messages for a subscriber until the subscription
        // has reached it. The connect is done, but the subscription can still
        // be in flight: give it time before the ready signal.
        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        auto ready = NewMessage();
        Check(Send(ready, "control") >= 0, "send ready signal");

        auto msg = NewMessage();
        Check(Receive(msg, "data") >= 0, "receive data");

        auto ack = NewMessage();
        Check(Send(ack, "control") >= 0, "send ack");

        Succeed();
    }
};

} // namespace fair::mq::test

#endif /* FAIR_MQ_TEST_SUB_H */
