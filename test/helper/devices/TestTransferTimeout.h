/********************************************************************************
 * Copyright (C) 2015-2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_TEST_TRANSFERTIMEOUT_H
#define FAIR_MQ_TEST_TRANSFERTIMEOUT_H

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
        bool sendMsgCanceling = false;
        bool receiveMsgCanceling = false;

        FairMQMessagePtr msg1(NewMessage());
        FairMQMessagePtr msg2(NewMessage());

        if (Send(msg1, "data-out", 0, 100) == -2)
        {
            LOG(info) << "send msg canceled";
            sendMsgCanceling = true;
        }
        else
        {
            LOG(error) << "send msg did not cancel";
        }

        if (Receive(msg2, "data-in", 0, 100) == -2)
        {
            LOG(info) << "receive msg canceled";
            receiveMsgCanceling = true;
        }
        else
        {
            LOG(error) << "receive msg did not cancel";
        }

        bool send1PartCanceling = false;
        bool receive1PartCanceling = false;

        FairMQParts parts1;
        parts1.AddPart(NewMessage(10));
        FairMQParts parts2;

        if (Send(parts1, "data-out", 0, 100) == -2)
        {
            LOG(info) << "send 1 part canceled";
            send1PartCanceling = true;
        }
        else
        {
            LOG(error) << "send 1 part did not cancel";
        }

        if (Receive(parts2, "data-in", 0, 100) == -2)
        {
            LOG(info) << "receive 1 part canceled";
            receive1PartCanceling = true;
        }
        else
        {
            LOG(error) << "receive 1 part did not cancel";
        }

        bool send2PartsCanceling = false;
        bool receive2PartsCanceling = false;

        FairMQParts parts3;
        parts3.AddPart(NewMessage(10));
        parts3.AddPart(NewMessage(10));
        FairMQParts parts4;

        if (Send(parts3, "data-out", 0, 100) == -2)
        {
            LOG(info) << "send 2 parts canceled";
            send2PartsCanceling = true;
        }
        else
        {
            LOG(error) << "send 2 parts did not cancel";
        }

        if (Receive(parts4, "data-in", 0, 100) == -2)
        {
            LOG(info) << "receive 2 parts canceled";
            receive2PartsCanceling = true;
        }
        else
        {
            LOG(error) << "receive 2 parts did not cancel";
        }

        if (sendMsgCanceling && receiveMsgCanceling && send1PartCanceling && receive1PartCanceling && send2PartsCanceling && receive2PartsCanceling)
        {
            LOG(info) << "Transfer timeout test successfull";
        }
    };
};

} // namespace test
} // namespace mq
} // namespace fair

#endif /* FAIR_MQ_TEST_TRANSFERTIMEOUT_H */
