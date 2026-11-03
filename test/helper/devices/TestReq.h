/********************************************************************************
 * Copyright (C) 2015-2023 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_TEST_REQ_H
#define FAIR_MQ_TEST_REQ_H

#include "TestDevice.h"

namespace fair::mq::test
{

class Req : public TestDevice
{
  protected:
    auto Run() -> void override
    {
        auto request = NewSimpleMessageFor("data", 0, "request");
        Check(Send(request, "data") == 7, "send request");

        auto reply = NewMessage();
        Check(Receive(reply, "data") == 5, "receive reply");
        Check(AsString(*reply) == "reply", "reply content");

        Succeed();
    }
};

} // namespace fair::mq::test

#endif /* FAIR_MQ_TEST_REQ_H */
