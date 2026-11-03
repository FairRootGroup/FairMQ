/********************************************************************************
 * Copyright (C) 2015-2022 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_TEST_PUSH_H
#define FAIR_MQ_TEST_PUSH_H

#include "TestDevice.h"

namespace fair::mq::test
{

class Push : public TestDevice
{
  protected:
    auto Run() -> void override
    {
        auto msg1 = NewMessageFor("data", 0);
        Check(Send(msg1, "data") == 0, "send empty message");
        auto msg2(NewSimpleMessageFor("data", 0, "testdata1234"));
        Check(Send(msg2, "data") == 12, "send text message");

        Succeed();
    }
};

} // namespace fair::mq::test

#endif /* FAIR_MQ_TEST_PUSH_H */
