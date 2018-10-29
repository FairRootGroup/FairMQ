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
        bool sendMsgCancelingAfter100ms = false;
        bool receiveMsgCancelingAfter100ms = false;

        bool sendMsgCancelingAfter0ms = false;
        bool receiveMsgCancelingAfter0ms = false;

        bool send1PartCancelingAfter100ms = false;
        bool receive1PartCancelingAfter100ms = false;

        bool send1PartCancelingAfter0ms = false;
        bool receive1PartCancelingAfter0ms = false;

        bool send2PartsCancelingAfter100ms = false;
        bool receive2PartsCancelingAfter100ms = false;

        bool send2PartsCancelingAfter0ms = false;
        bool receive2PartsCancelingAfter0ms = false;

        FairMQMessagePtr msg1(NewMessage());
        FairMQMessagePtr msg2(NewMessage());

        if (Send(msg1, "data-out", 0, 100) == -2) {
            LOG(info) << "send msg canceled (100ms)";
            sendMsgCancelingAfter100ms = true;
        } else {
            LOG(error) << "send msg did not cancel (100ms)";
        }

        if (Receive(msg2, "data-in", 0, 100) == -2) {
            LOG(info) << "receive msg canceled (100ms)";
            receiveMsgCancelingAfter100ms = true;
        } else {
            LOG(error) << "receive msg did not cancel (100ms)";
        }

        if (Send(msg1, "data-out", 0, 0) == -2) {
            LOG(info) << "send msg canceled (0ms)";
            sendMsgCancelingAfter0ms = true;
        } else {
            LOG(error) << "send msg did not cancel (0ms)";
        }

        if (Receive(msg2, "data-in", 0, 0) == -2) {
            LOG(info) << "receive msg canceled (0ms)";
            receiveMsgCancelingAfter0ms = true;
        } else {
            LOG(error) << "receive msg did not cancel (0ms)";
        }

        FairMQParts parts1;
        parts1.AddPart(NewMessage(10));
        FairMQParts parts2;

        if (Send(parts1, "data-out", 0, 100) == -2) {
            LOG(info) << "send 1 part canceled (100ms)";
            send1PartCancelingAfter100ms = true;
        } else {
            LOG(error) << "send 1 part did not cancel (100ms)";
        }

        if (Receive(parts2, "data-in", 0, 100) == -2) {
            LOG(info) << "receive 1 part canceled (100ms)";
            receive1PartCancelingAfter100ms = true;
        } else {
            LOG(error) << "receive 1 part did not cancel (100ms)";
        }

        if (Send(parts1, "data-out", 0, 0) == -2) {
            LOG(info) << "send 1 part canceled (0ms)";
            send1PartCancelingAfter0ms = true;
        } else {
            LOG(error) << "send 1 part did not cancel (0ms)";
        }

        if (Receive(parts2, "data-in", 0, 0) == -2) {
            LOG(info) << "receive 1 part canceled (0ms)";
            receive1PartCancelingAfter0ms = true;
        } else {
            LOG(error) << "receive 1 part did not cancel (0ms)";
        }

        FairMQParts parts3;
        parts3.AddPart(NewMessage(10));
        parts3.AddPart(NewMessage(10));
        FairMQParts parts4;

        if (Send(parts3, "data-out", 0, 100) == -2) {
            LOG(info) << "send 2 parts canceled (100ms)";
            send2PartsCancelingAfter100ms = true;
        } else {
            LOG(error) << "send 2 parts did not cancel (100ms)";
        }

        if (Receive(parts4, "data-in", 0, 100) == -2) {
            LOG(info) << "receive 2 parts canceled (100ms)";
            receive2PartsCancelingAfter100ms = true;
        } else {
            LOG(error) << "receive 2 parts did not cancel (100ms)";
        }

        if (Send(parts3, "data-out", 0, 0) == -2) {
            LOG(info) << "send 2 parts canceled (0ms)";
            send2PartsCancelingAfter0ms = true;
        } else {
            LOG(error) << "send 2 parts did not cancel (0ms)";
        }

        if (Receive(parts4, "data-in", 0, 0) == -2) {
            LOG(info) << "receive 2 parts canceled (0ms)";
            receive2PartsCancelingAfter0ms = true;
        } else {
            LOG(error) << "receive 2 parts did not cancel (0ms)";
        }

        if (sendMsgCancelingAfter100ms &&
            receiveMsgCancelingAfter100ms &&
            sendMsgCancelingAfter0ms &&
            receiveMsgCancelingAfter0ms &&
            send1PartCancelingAfter100ms &&
            receive1PartCancelingAfter100ms &&
            send1PartCancelingAfter0ms &&
            receive1PartCancelingAfter0ms &&
            send2PartsCancelingAfter100ms &&
            receive2PartsCancelingAfter100ms &&
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
