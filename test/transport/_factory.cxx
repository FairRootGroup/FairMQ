/********************************************************************************
 *    Copyright (C) 2026 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <fairmq/Channel.h>
#include <fairmq/ProgOptions.h>
#include <fairmq/Socket.h>
#include <fairmq/TransportFactory.h>
#include <fairmq/tools/Unique.h>

#include <helper/Transport.h>
#include <helper/TransportParam.h>

#include <gtest/gtest.h>

#include <cstdint>
#include <cstring>
#include <string>

namespace
{

using namespace std;
using namespace fair::mq;

auto AsString(Message const& msg) -> string
{
    return {static_cast<char const*>(msg.GetData()), msg.GetSize()};
}

TEST(TransportFactory, UnknownTransportThrows)
{
    ProgOptions config;
    test::Configure(config);
    EXPECT_THROW(TransportFactory::CreateTransportFactory("carrier-pigeon", tools::Uuid(), &config),
                 TransportFactoryError);
}

TEST(TransportFactory, EmptyIdGetsAUuid)
{
    ProgOptions config;
    test::Configure(config);
    auto factory = TransportFactory::CreateTransportFactory("zeromq", "", &config);
    EXPECT_FALSE(factory->GetId().empty());
    EXPECT_EQ(TransportFactory::CreateTransportFactory("zeromq", "my-id", &config)->GetId(), "my-id");
}

TEST(TransportFactory, ShmemRejectsPubSub)
{
    ProgOptions config;
    test::Configure(config);
    auto shm = TransportFactory::CreateTransportFactory("shmem", tools::Uuid(), &config);
    EXPECT_THROW(shm->CreateSocket("pub", "pub"), SocketError);
    EXPECT_THROW(shm->CreateSocket("sub", "sub"), SocketError);

    auto zmq = TransportFactory::CreateTransportFactory("zeromq", tools::Uuid(), &config);
    EXPECT_NE(zmq->CreateSocket("pub", "pub"), nullptr);
    EXPECT_NE(zmq->CreateSocket("sub", "sub"), nullptr);
}

using Factory = test::TransportTest;

TEST_P(Factory, Type)
{
    ProgOptions config;
    test::Configure(config, tools::Uuid(), GetParam().expandedShmMetadata);
    auto factory = TransportFactory::CreateTransportFactory(GetParam().transport, tools::Uuid(), &config);
    EXPECT_EQ(factory->GetType(), GetParam().transport == "zeromq" ? Transport::ZMQ : Transport::SHM);
}

TEST_P(Factory, ResumeAfterInterrupt)
{
    ProgOptions config;
    test::Configure(config, tools::Uuid(), GetParam().expandedShmMetadata);
    auto factory = TransportFactory::CreateTransportFactory(GetParam().transport, tools::Uuid(), &config);

    Channel push("push", "push", factory);
    Channel pull("pull", "pull", factory);
    test::IpcAddress const address("resume_after_interrupt");
    ASSERT_TRUE(pull.Bind(address));
    ASSERT_TRUE(push.Connect(address));

    factory->Interrupt();
    auto in(pull.NewMessage());
    EXPECT_EQ(pull.Receive(in), static_cast<int64_t>(TransferCode::interrupted));

    factory->Resume();
    auto out(push.NewSimpleMessage("after resume"));
    ASSERT_EQ(push.Send(out, 5000), 12);
    ASSERT_EQ(pull.Receive(in, 5000), 12);
    EXPECT_EQ(AsString(*in), "after resume");
}

TEST_P(Factory, SimpleAndStaticMessages)
{
    ProgOptions config;
    test::Configure(config, tools::Uuid(), GetParam().expandedShmMetadata);
    auto factory = TransportFactory::CreateTransportFactory(GetParam().transport, tools::Uuid(), &config);

    Channel push("push", "push", factory);
    Channel pull("pull", "pull", factory);
    test::IpcAddress const address("simple_and_static");
    ASSERT_TRUE(pull.Bind(address));
    ASSERT_TRUE(push.Connect(address));

    auto receive = [&]() {
        auto msg(pull.NewMessage());
        EXPECT_GE(pull.Receive(msg, 5000), 0);
        return msg;
    };

    // NewSimpleMessage() copies the data: the source can change after the call.
    string text("simple");
    auto simple(factory->NewSimpleMessage(text));
    text = "changed";
    ASSERT_EQ(push.Send(simple, 5000), 6);
    EXPECT_EQ(AsString(*receive()), "simple");

    auto literal(factory->NewSimpleMessage("literal"));
    ASSERT_EQ(push.Send(literal, 5000), 7);
    EXPECT_EQ(AsString(*receive()), "literal");

    uint64_t const value = 0x0123456789abcdef;
    auto number(factory->NewSimpleMessage(value));
    ASSERT_EQ(push.Send(number, 5000), static_cast<int64_t>(sizeof(value)));
    auto numberIn = receive();
    ASSERT_EQ(numberIn->GetSize(), sizeof(value));
    uint64_t received = 0;
    memcpy(&received, numberIn->GetData(), sizeof(received));
    EXPECT_EQ(received, value);

    // NewStaticMessage() uses the buffer of the caller, which must outlive the message.
    static string const staticText("static");
    auto staticMsg(factory->NewStaticMessage(staticText));
    EXPECT_EQ(staticMsg->GetSize(), staticText.size());
    if (GetParam().transport == "zeromq") {
        EXPECT_EQ(staticMsg->GetData(), staticText.data()); // zeromq does not copy
    }
    ASSERT_EQ(push.Send(staticMsg, 5000), 6);
    EXPECT_EQ(AsString(*receive()), "static");
}

INSTANTIATE_TEST_SUITE_P(, Factory, test::AllTransports(), test::TransportParamName());

} // namespace
