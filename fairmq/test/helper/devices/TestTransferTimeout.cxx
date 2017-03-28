/********************************************************************************
 * Copyright (C) 2015-2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * @file fairmq/test/helper/devices/TestTransferTimeout.cxx
 */

#include <FairMQDevice.h>
#include <FairMQLogger.h>

namespace fair
{
namespace mq
{
namespace test
{

class TransferTimeout : public FairMQDevice
{
  protected:
    auto Run() -> void override
    {
        auto sendCanceling = false;
        auto receiveCanceling = false;

        auto msg1 = FairMQMessagePtr{NewMessage()};
        auto msg2 = FairMQMessagePtr{NewMessage()};

        if (Send(msg1, "data-out", 0, 100) == -2)
        {
            LOG(INFO) << "send canceled";
            sendCanceling = true;
        }
        else
        {
            LOG(ERROR) << "send did not cancel";
        }

        if (Receive(msg2, "data-in", 0, 100) == -2)
        {
            LOG(INFO) << "receive canceled";
            receiveCanceling = true;
        }
        else
        {
            LOG(ERROR) << "receive did not cancel";
        }

        if (sendCanceling && receiveCanceling)
        {
            LOG(INFO) << "Transfer timeout test successfull";
        }
    };
};

} // namespace test
} // namespace mq
} // namespace fair
