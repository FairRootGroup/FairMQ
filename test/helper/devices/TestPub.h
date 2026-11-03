/********************************************************************************
 * Copyright (C) 2015-2022 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_TEST_PUB_H
#define FAIR_MQ_TEST_PUB_H

#include "TestDevice.h"

namespace fair::mq::test
{

/// Publishes one message after both Sub devices report that they are ready,
/// then waits for an ack from each.
class Pub : public TestDevice
{
  protected:
    auto Run() -> void override
    {
        for (int i = 0; i < 2; ++i) {
            auto ready = NewMessage();
            Check(Receive(ready, "control") >= 0, "receive ready signal");
        }

        auto msg = NewMessage();
        Check(Send(msg, "data") >= 0, "publish data");

        for (int i = 0; i < 2; ++i) {
            auto ack = NewMessage();
            Check(Receive(ack, "control") >= 0, "receive ack");
        }

        Succeed();
    }
};

} // namespace fair::mq::test

#endif /* FAIR_MQ_TEST_PUB_H */
