/********************************************************************************
 * Copyright (C) 2015-2020 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
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
        bool sendMsgCancelingAfter200ms = false;
        bool receiveMsgCancelingAfter200ms = false;

        bool sendMsgCancelingAfter0ms = false;
        bool receiveMsgCancelingAfter0ms = false;

        bool send1PartCancelingAfter200ms = false;
        bool receive1PartCancelingAfter200ms = false;

        bool send1PartCancelingAfter0ms = false;
        bool receive1PartCancelingAfter0ms = false;

        bool send2PartsCancelingAfter200ms = false;
        bool receive2PartsCancelingAfter200ms = false;

        bool send2PartsCancelingAfter0ms = false;
        bool receive2PartsCancelingAfter0ms = false;

        FairMQMessagePtr msg1(NewMessage());
        FairMQMessagePtr msg2(NewMessage());

        if (Send(msg1, "data-out", 0, 200) == static_cast<int>(TransferCode::timeout)) {
            LOG(info) << "send msg canceled (200ms)";
            sendMsgCancelingAfter200ms = true;
        } else {
            LOG(error) << "send msg did not cancel (200ms)";
        }

        if (Receive(msg2, "data-in", 0, 200) == static_cast<int>(TransferCode::timeout)) {
            LOG(info) << "receive msg canceled (200ms)";
            receiveMsgCancelingAfter200ms = true;
        } else {
            LOG(error) << "receive msg did not cancel (200ms)";
        }

        if (Send(msg1, "data-out", 0, 0) == static_cast<int>(TransferCode::timeout)) {
            LOG(info) << "send msg canceled (0ms)";
            sendMsgCancelingAfter0ms = true;
        } else {
            LOG(error) << "send msg did not cancel (0ms)";
        }

        if (Receive(msg2, "data-in", 0, 0) == static_cast<int>(TransferCode::timeout)) {
            LOG(info) << "receive msg canceled (0ms)";
            receiveMsgCancelingAfter0ms = true;
        } else {
            LOG(error) << "receive msg did not cancel (0ms)";
        }

        FairMQParts parts1;
        parts1.AddPart(NewMessage(10));
        FairMQParts parts2;

        if (Send(parts1, "data-out", 0, 200) == static_cast<int>(TransferCode::timeout)) {
            LOG(info) << "send 1 part canceled (200ms)";
            send1PartCancelingAfter200ms = true;
        } else {
            LOG(error) << "send 1 part did not cancel (200ms)";
        }

        if (Receive(parts2, "data-in", 0, 200) == static_cast<int>(TransferCode::timeout)) {
            LOG(info) << "receive 1 part canceled (200ms)";
            receive1PartCancelingAfter200ms = true;
        } else {
            LOG(error) << "receive 1 part did not cancel (200ms)";
        }

        if (Send(parts1, "data-out", 0, 0) == static_cast<int>(TransferCode::timeout)) {
            LOG(info) << "send 1 part canceled (0ms)";
            send1PartCancelingAfter0ms = true;
        } else {
            LOG(error) << "send 1 part did not cancel (0ms)";
        }

        if (Receive(parts2, "data-in", 0, 0) == static_cast<int>(TransferCode::timeout)) {
            LOG(info) << "receive 1 part canceled (0ms)";
            receive1PartCancelingAfter0ms = true;
        } else {
            LOG(error) << "receive 1 part did not cancel (0ms)";
        }

        FairMQParts parts3;
        parts3.AddPart(NewMessage(10));
        parts3.AddPart(NewMessage(10));
        FairMQParts parts4;

        if (Send(parts3, "data-out", 0, 200) == static_cast<int>(TransferCode::timeout)) {
            LOG(info) << "send 2 parts canceled (200ms)";
            send2PartsCancelingAfter200ms = true;
        } else {
            LOG(error) << "send 2 parts did not cancel (200ms)";
        }

        if (Receive(parts4, "data-in", 0, 200) == static_cast<int>(TransferCode::timeout)) {
            LOG(info) << "receive 2 parts canceled (200ms)";
            receive2PartsCancelingAfter200ms = true;
        } else {
            LOG(error) << "receive 2 parts did not cancel (200ms)";
        }

        if (Send(parts3, "data-out", 0, 0) == static_cast<int>(TransferCode::timeout)) {
            LOG(info) << "send 2 parts canceled (0ms)";
            send2PartsCancelingAfter0ms = true;
        } else {
            LOG(error) << "send 2 parts did not cancel (0ms)";
        }

        if (Receive(parts4, "data-in", 0, 0) == static_cast<int>(TransferCode::timeout)) {
            LOG(info) << "receive 2 parts canceled (0ms)";
            receive2PartsCancelingAfter0ms = true;
        } else {
            LOG(error) << "receive 2 parts did not cancel (0ms)";
        }

        if (sendMsgCancelingAfter200ms &&
            receiveMsgCancelingAfter200ms &&
            sendMsgCancelingAfter0ms &&
            receiveMsgCancelingAfter0ms &&
            send1PartCancelingAfter200ms &&
            receive1PartCancelingAfter200ms &&
            send1PartCancelingAfter0ms &&
            receive1PartCancelingAfter0ms &&
            send2PartsCancelingAfter200ms &&
            receive2PartsCancelingAfter200ms &&
            send2PartsCancelingAfter0ms &&
            receive2PartsCancelingAfter0ms)
        {
            LOG(info) << "Transfer timeout test successfull";
        }
    };
};

} // namespace test
} // namespace mq
} // namespace fair

#endif /* FAIR_MQ_TEST_TRANSFERTIMEOUT_H */
