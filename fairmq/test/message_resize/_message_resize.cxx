/********************************************************************************
 *    Copyright (C) 2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <FairMQChannel.h>
#include <FairMQLogger.h>
#include <FairMQTransportFactory.h>
#include <options/FairMQProgOptions.h>
#include <fairmq/Tools.h>

#include <gtest/gtest.h>

#include <string>

namespace
{

using namespace std;

auto RunPushPullWithMsgResize(string transport, string address) -> void {

    size_t session{fair::mq::tools::UuidHash()};

    FairMQProgOptions config;
    config.SetValue<string>("session", to_string(session));
    config.SetValue<int>("io-threads", 1);
    config.SetValue<size_t>("shm-segment-size", 20000000);

    auto factory = FairMQTransportFactory::CreateTransportFactory(transport, fair::mq::tools::Uuid(), &config);

    FairMQChannel push{"Push", "push", factory};
    push.Bind(address);

    FairMQChannel pull{"Pull", "pull", factory};
    pull.Connect(address);

    FairMQMessagePtr outMsg(push.NewMessage(1000));
    ASSERT_EQ(outMsg->GetSize(), 1000);
    outMsg->SetUsedSize(500);
    ASSERT_EQ(outMsg->GetSize(), 500);
    outMsg->SetUsedSize(250);
    ASSERT_EQ(outMsg->GetSize(), 250);
    FairMQMessagePtr msgCopy(push.NewMessage());
    msgCopy->Copy(outMsg);
    ASSERT_EQ(msgCopy->GetSize(), 250);

    ASSERT_EQ(push.Send(outMsg), 250);

    FairMQMessagePtr inMsg(pull.NewMessage());
    ASSERT_EQ(pull.Receive(inMsg), 250);
    ASSERT_EQ(inMsg->GetSize(), 250);
}

TEST(MessageResize, ZeroMQ)
{
    RunPushPullWithMsgResize("zeromq", "ipc://test_message_resize");
}

TEST(MessageResize, shmem)
{
    RunPushPullWithMsgResize("shmem", "ipc://test_message_resize");
}

#ifdef NANOMSG_FOUND
TEST(MessageResize, nanomsg)
{
    RunPushPullWithMsgResize("nanomsg", "ipc://test_message_resize");
}
#endif /* NANOMSG_FOUND */

} // namespace
