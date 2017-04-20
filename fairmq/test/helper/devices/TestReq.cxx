/********************************************************************************
 * Copyright (C) 2015-2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <FairMQDevice.h>
#include <FairMQLogger.h>

namespace fair
{
namespace mq
{
namespace test
{

class Req : public FairMQDevice
{
  protected:
    auto Init() -> void override
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    auto Reset() -> void override
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    auto Run() -> void override
    {
        auto request = FairMQMessagePtr{NewMessage()};
        Send(request, "data");

        auto reply = FairMQMessagePtr{NewMessage()};
        if (Receive(reply, "data") >= 0)
        {
            LOG(INFO) << "received reply";
        }
    };
};

} // namespace test
} // namespace mq
} // namespace fair
