/********************************************************************************
 * Copyright (C) 2015-2023 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_TEST_REP_H
#define FAIR_MQ_TEST_REP_H

#include "TestDevice.h"

namespace fair::mq::test
{

/// Answers two requests (one from each Req device).
class Rep : public TestDevice
{
  protected:
    auto Run() -> void override
    {
        for (int i = 0; i < 2; ++i) {
            auto request = NewMessage();
            Check(Receive(request, "data") == 7, "receive request");
            Check(AsString(*request) == "request", "request content");
            auto reply = NewSimpleMessageFor("data", 0, "reply");
            Check(Send(reply, "data") == 5, "send reply");
        }

        Succeed();
    }
};

} // namespace fair::mq::test

#endif /* FAIR_MQ_TEST_REP_H */
