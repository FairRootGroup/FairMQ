/********************************************************************************
 * Copyright (C) 2015-2022 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_TEST_PUSH_H
#define FAIR_MQ_TEST_PUSH_H

#include <fairmq/Device.h>
#include <thread>

namespace fair::mq::test
{

class Push : public Device
{
  protected:
    auto InitTask() -> void override
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    auto Run() -> void override
    {
        // empty message
        auto msg1 = NewMessageFor("data", 0);
        Send(msg1, "data");
        // message with short text data
        auto msg2(NewSimpleMessageFor("data", 0, "testdata1234"));
        Send(msg2, "data");
    };
};

} // namespace fair::mq::test

#endif /* FAIR_MQ_TEST_PUSH_H */
