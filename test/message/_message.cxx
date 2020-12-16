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
#include <fairmq/ProgOptions.h>
#include <fairmq/Tools.h>

#include <gtest/gtest.h>

#include <string>
#include <cstdint>

namespace
{

using namespace std;

void RunPushPullWithMsgResize(const string& transport, const string& _address)
{
    size_t session{fair::mq::tools::UuidHash()};
    std::string address(fair::mq::tools::ToString(_address, "_", transport));

    fair::mq::ProgOptions config;
    config.SetProperty<string>("session", to_string(session));

    auto factory = FairMQTransportFactory::CreateTransportFactory(transport, fair::mq::tools::Uuid(), &config);

    FairMQChannel push{"Push", "push", factory};
    push.Bind(address);

    FairMQChannel pull{"Pull", "pull", factory};
    pull.Connect(address);

    FairMQMessagePtr outMsg(push.NewMessage(1000));
    ASSERT_EQ(outMsg->GetSize(), 1000);
    memcpy(outMsg->GetData(), "ABC", 3);
    ASSERT_EQ(outMsg->SetUsedSize(500), true);
    ASSERT_EQ(outMsg->SetUsedSize(500), true);
    ASSERT_EQ(outMsg->SetUsedSize(700), false);
    ASSERT_EQ(outMsg->GetSize(), 500);
    // check if the data is still intact
    ASSERT_EQ(static_cast<char*>(outMsg->GetData())[0], 'A');
    ASSERT_EQ(static_cast<char*>(outMsg->GetData())[1], 'B');
    ASSERT_EQ(static_cast<char*>(outMsg->GetData())[2], 'C');
    ASSERT_EQ(outMsg->SetUsedSize(250), true);
    ASSERT_EQ(outMsg->GetSize(), 250);
    ASSERT_EQ(static_cast<char*>(outMsg->GetData())[0], 'A');
    ASSERT_EQ(static_cast<char*>(outMsg->GetData())[1], 'B');
    ASSERT_EQ(static_cast<char*>(outMsg->GetData())[2], 'C');
    FairMQMessagePtr msgCopy(push.NewMessage());
    msgCopy->Copy(*outMsg);
    ASSERT_EQ(msgCopy->GetSize(), 250);

    ASSERT_EQ(push.Send(outMsg), 250);

    FairMQMessagePtr inMsg(pull.NewMessage());
    ASSERT_EQ(pull.Receive(inMsg), 250);
    ASSERT_EQ(inMsg->GetSize(), 250);
    ASSERT_EQ(static_cast<char*>(inMsg->GetData())[0], 'A');
    ASSERT_EQ(static_cast<char*>(inMsg->GetData())[1], 'B');
    ASSERT_EQ(static_cast<char*>(inMsg->GetData())[2], 'C');
}

void RunMsgRebuild(const string& transport)
{
    size_t session{fair::mq::tools::UuidHash()};

    fair::mq::ProgOptions config;
    config.SetProperty<string>("session", to_string(session));

    auto factory = FairMQTransportFactory::CreateTransportFactory(transport, fair::mq::tools::Uuid(), &config);

    FairMQMessagePtr msg(factory->CreateMessage());
    EXPECT_EQ(msg->GetSize(), 0);
    msg->Rebuild(100);
    EXPECT_EQ(msg->GetSize(), 100);
    string* str = new string("asdf");
    msg->Rebuild(const_cast<char*>(str->c_str()),
                 str->length(),
                 [](void* /*data*/, void* obj) { delete static_cast<string*>(obj); },
                 str);
    EXPECT_NE(msg->GetSize(), 100);
    EXPECT_EQ(msg->GetSize(), string("asdf").length());
    EXPECT_EQ(string(static_cast<char*>(msg->GetData()), msg->GetSize()), string("asdf"));
}

void Alignment(const string& transport, const string& _address)
{
    size_t session{fair::mq::tools::UuidHash()};
    std::string address(fair::mq::tools::ToString(_address, "_", transport));

    fair::mq::ProgOptions config;
    config.SetProperty<string>("session", to_string(session));

    auto factory = FairMQTransportFactory::CreateTransportFactory(transport, fair::mq::tools::Uuid(), &config);

    FairMQChannel push{"Push", "push", factory};
    push.Bind(address);

    FairMQChannel pull{"Pull", "pull", factory};
    pull.Connect(address);

    size_t alignment = 64;

    FairMQMessagePtr outMsg(push.NewMessage(100, fair::mq::Alignment{alignment}));
    ASSERT_EQ(reinterpret_cast<uintptr_t>(outMsg->GetData()) % alignment, 0);
    ASSERT_EQ(push.Send(outMsg), 100);

    FairMQMessagePtr inMsg(pull.NewMessage());
    ASSERT_EQ(pull.Receive(inMsg), 100);
    // ASSERT_EQ(reinterpret_cast<uintptr_t>(inMsg->GetData()) % alignment, 0);
}

void EmptyMessage(const string& transport, const string& _address)
{
    size_t session{fair::mq::tools::UuidHash()};
    std::string address(fair::mq::tools::ToString(_address, "_", transport));

    fair::mq::ProgOptions config;
    config.SetProperty<string>("session", to_string(session));

    auto factory = FairMQTransportFactory::CreateTransportFactory(transport, fair::mq::tools::Uuid(), &config);

    FairMQChannel push{"Push", "push", factory};
    push.Bind(address);

    FairMQChannel pull{"Pull", "pull", factory};
    pull.Connect(address);

    FairMQMessagePtr outMsg(push.NewMessage());
    ASSERT_EQ(outMsg->GetData(), nullptr);
    ASSERT_EQ(push.Send(outMsg), 0);

    FairMQMessagePtr inMsg(pull.NewMessage());
    ASSERT_EQ(pull.Receive(inMsg), 0);
    ASSERT_EQ(inMsg->GetData(), nullptr);
}

TEST(Resize, zeromq)
{
    RunPushPullWithMsgResize("zeromq", "ipc://test_message_resize");
}

TEST(Resize, shmem)
{
    RunPushPullWithMsgResize("shmem", "ipc://test_message_resize");
}

TEST(Rebuild, zeromq)
{
    RunMsgRebuild("zeromq");
}

TEST(Rebuild, shmem)
{
    RunMsgRebuild("shmem");
}

TEST(Alignment, shmem)
{
    Alignment("shmem", "ipc://test_message_alignment");
}

TEST(Alignment, zeromq)
{
    Alignment("zeromq", "ipc://test_message_alignment");
}

TEST(EmptyMessage, zeromq)
{
    EmptyMessage("zeromq", "ipc://test_empty_message");
}

TEST(EmptyMessage, shmem)
{
    EmptyMessage("shmem", "ipc://test_empty_message");
}

} // namespace
