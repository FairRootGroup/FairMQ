/********************************************************************************
 * Copyright (C) 2015-2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
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

class Push : public FairMQDevice
{
  protected:
    auto InitTask() -> void override
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    auto Run() -> void override
    {
        auto msg = FairMQMessagePtr{NewMessage()};
        Send(msg, "data");
    };
};

} // namespace fair::mq::test

#endif /* FAIR_MQ_TEST_PUSH_H */
