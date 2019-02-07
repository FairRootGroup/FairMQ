/********************************************************************************
 * Copyright (C) 2015-2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_TEST_POLLOUT_H
#define FAIR_MQ_TEST_POLLOUT_H

#include <FairMQDevice.h>
#include <thread>

namespace fair
{
namespace mq
{
namespace test
{

class PollOut : public FairMQDevice
{
  protected:
    auto InitTask() -> void override
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    auto Run() -> void override
    {
        auto msg1 = FairMQMessagePtr{NewMessage()};
        auto msg2 = FairMQMessagePtr{NewMessage()};
        Send(msg1, "data1");
        Send(msg2, "data2");
    };
};

} // namespace test
} // namespace mq
} // namespace fair

#endif /* FAIR_MQ_TEST_POLLOUT_H */
