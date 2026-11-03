/********************************************************************************
 * Copyright (C) 2015-2022 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_TEST_PULL_H
#define FAIR_MQ_TEST_PULL_H

#include "TestDevice.h"

namespace fair::mq::test
{

class Pull : public TestDevice
{
  protected:
    auto Run() -> void override
    {
        auto msg1 = NewMessageFor("data", 0);
        Check(Receive(msg1, "data") == 0, "receive empty message");
        auto msg2 = NewMessageFor("data", 0);
        Check(Receive(msg2, "data") == 12, "receive text message");
        Check(AsString(*msg2) == "testdata1234", "text message content");

        Succeed();
    }
};

} // namespace fair::mq::test

#endif /* FAIR_MQ_TEST_PULL_H */
