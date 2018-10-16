/********************************************************************************
 * Copyright (C) 2015-2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <FairMQDevice.h>
#include <FairMQLogger.h>
#include <thread>

namespace fair
{
namespace mq
{
namespace test
{

using namespace std;

class Pull : public FairMQDevice
{
  protected:
    auto Init() -> void override
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    auto Run() -> void override
    {
        auto msg = FairMQMessagePtr{NewMessage()};

        if (Receive(msg, "data") >= 0)
        {
            LOG(info) << "PUSH-PULL test successfull";
        }
    };
};

} // namespace test
} // namespace mq
} // namespace fair
