/********************************************************************************
 * Copyright (C) 2015-2022 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_TEST_PAIRRIGHT_H
#define FAIR_MQ_TEST_PAIRRIGHT_H

#include "TestDevice.h"

namespace fair::mq::test
{

class PairRight : public TestDevice
{
  protected:
    auto Run() -> void override
    {
        // empty message ping pong
        auto msg1(NewMessageFor("data", 0));
        Check(Receive(msg1, "data") >= 0, "receive empty message 1");
        auto msg2(NewMessageFor("data", 0));
        Check(Send(msg2, "data") >= 0, "send empty message 2");
        auto msg3(NewMessageFor("data", 0));
        Check(Receive(msg3, "data") >= 0, "receive empty message 3");
        auto msg4(NewMessageFor("data", 0));
        Check(Send(msg4, "data") >= 0, "send empty message 4");

        // message with short text data
        auto msg5(NewMessageFor("data", 0));
        Check(Receive(msg5, "data") == 12, "receive text message");
        Check(AsString(*msg5) == "testdata1234", "text message content");
        auto msg6(NewSimpleMessageFor("data", 0, "testdata1234"));
        Check(Send(msg6, "data") == 12, "send text message");

        Succeed();
    }
};

} // namespace fair::mq::test

#endif /* FAIR_MQ_TEST_PAIRRIGHT_H */
