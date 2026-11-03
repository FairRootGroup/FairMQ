/********************************************************************************
 * Copyright (C) 2015-2022 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_TEST_POLLOUT_H
#define FAIR_MQ_TEST_POLLOUT_H

#include "TestDevice.h"

namespace fair::mq::test
{

class PollOut : public TestDevice
{
  protected:
    auto Run() -> void override
    {
        auto msg1 = NewMessage();
        Check(Send(msg1, "data1") >= 0, "send on data1");
        auto msg2 = NewMessage();
        Check(Send(msg2, "data2") >= 0, "send on data2");

        Succeed();
    }
};

} // namespace fair::mq::test

#endif /* FAIR_MQ_TEST_POLLOUT_H */
