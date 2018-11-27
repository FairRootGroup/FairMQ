/********************************************************************************
 *    Copyright (C) 2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <gtest/gtest.h>
#include <FairMQChannel.h>
#include <FairMQParts.h>
#include <FairMQLogger.h>
#include <FairMQTransportFactory.h>
#include <fairmq/Tools.h>
#include <options/FairMQProgOptions.h>

#include <algorithm>
#include <memory>
#include <sstream>
#include <string>
#include <thread>

namespace
{

using namespace std;

auto RunSingleThreadedMultipart(string transport, string address) -> void {

    size_t session{fair::mq::tools::UuidHash()};

    FairMQProgOptions config;
    config.SetValue<string>("session", std::to_string(session));
    auto factory = FairMQTransportFactory::CreateTransportFactory(transport, fair::mq::tools::Uuid(), &config);
    FairMQTransportFactory* factoryptr = factory.get();
    auto push = FairMQChannel{"Push", "push", factory};
    ASSERT_TRUE(push.Bind(address));
    auto pull = FairMQChannel{"Pull", "pull", factory};
    pull.Connect(address);

    // TODO validate that fTransportFactory is not nullptr
    // TODO validate that fSocket is not nullptr
    ASSERT_TRUE(push.Validate());
    ASSERT_TRUE(pull.Validate());

    {
        auto sentMsg = FairMQParts{};
        sentMsg.AddPart(push.NewSimpleMessage("1"));
        sentMsg.AddPart(push.NewSimpleMessage("2"));
        sentMsg.AddPart(push.NewSimpleMessage("3"));

        ASSERT_GE(push.Send(sentMsg), 0);
    }

    auto receivedMsg = FairMQParts{};
    ASSERT_GE(pull.Receive(receivedMsg), 0);

    stringstream out;
    for_each(receivedMsg.cbegin(), receivedMsg.cend(), [&out,&factoryptr](const FairMQMessagePtr& part) {
        out << string{static_cast<char*>(part->GetData()), part->GetSize()};
        ASSERT_EQ(part->GetTransport(),factoryptr);
    });
    ASSERT_EQ(out.str(), "123");
}

auto RunMultiThreadedMultipart(string transport, string address) -> void
{
    size_t session{fair::mq::tools::UuidHash()};

    FairMQProgOptions config;
    config.SetValue<string>("session", std::to_string(session));
    config.SetValue<int>("io-threads", 1);
    config.SetValue<size_t>("shm-segment-size", 20000000);
    auto factory = FairMQTransportFactory::CreateTransportFactory(transport, fair::mq::tools::Uuid(), &config);
    auto push = FairMQChannel{"Push", "push", factory};
    ASSERT_TRUE(push.Bind(address));
    auto pull = FairMQChannel{"Pull", "pull", factory};
    pull.Connect(address);

    auto pusher = thread{[&push](){
        ASSERT_TRUE(push.Validate());

        auto sentMsg = FairMQParts{};
        sentMsg.AddPart(push.NewSimpleMessage("1"));
        sentMsg.AddPart(push.NewSimpleMessage("2"));
        sentMsg.AddPart(push.NewSimpleMessage("3"));

        ASSERT_GE(push.Send(sentMsg), 0);
    }};

    auto puller = thread{[&pull](){
        ASSERT_TRUE(pull.Validate());

        auto receivedMsg = FairMQParts{};
        ASSERT_GE(pull.Receive(receivedMsg), 0);

        stringstream out;
        for_each(receivedMsg.cbegin(), receivedMsg.cend(), [&out](const FairMQMessagePtr& part) {
            out << string{static_cast<char*>(part->GetData()), part->GetSize()};
        });
        ASSERT_EQ(out.str(), "123");
    }};

    pusher.join();
    puller.join();
}

TEST(PushPull, ST_ZeroMQ__inproc_Multipart)
{
    RunSingleThreadedMultipart("zeromq", "inproc://test");
}

TEST(PushPull, ST_Shmem___inproc_Multipart)
{
    RunSingleThreadedMultipart("shmem", "inproc://test");
}

#ifdef BUILD_NANOMSG_TRANSPORT
TEST(PushPull, ST_Nanomsg_inproc_Multipart)
{
    RunSingleThreadedMultipart("nanomsg", "inproc://test");
}
#endif /* BUILD_NANOMSG_TRANSPORT */

TEST(PushPull, ST_ZeroMQ__ipc____Multipart)
{
    RunSingleThreadedMultipart("zeromq", "ipc://test_ST_ZeroMQ__ipc____Multipart");
}

TEST(PushPull, ST_Shmen___ipc____Multipart)
{
    RunSingleThreadedMultipart("shmem", "ipc://test_ST_Shmen___ipc____Multipart");
}

#ifdef BUILD_NANOMSG_TRANSPORT
TEST(PushPull, ST_Nanomsg_ipc____Multipart)
{
    RunSingleThreadedMultipart("nanomsg", "ipc://test_ST_Nanomsg_ipc____Multipart");
}
#endif /* BUILD_NANOMSG_TRANSPORT */

TEST(PushPull, MT_ZeroMQ__inproc_Multipart)
{
    RunMultiThreadedMultipart("zeromq", "inproc://test");
}

TEST(PushPull, MT_Shmem___inproc_Multipart)
{
    RunMultiThreadedMultipart("shmem", "inproc://test");
}

#ifdef BUILD_NANOMSG_TRANSPORT
TEST(PushPull, MT_Nanomsg_inproc_Multipart)
{
    RunMultiThreadedMultipart("nanomsg", "inproc://test");
}
#endif /* BUILD_NANOMSG_TRANSPORT */

TEST(PushPull, MT_ZeroMQ__ipc____Multipart)
{
    RunMultiThreadedMultipart("zeromq", "ipc://test_MT_ZeroMQ__ipc____Multipart");
}

TEST(PushPull, MT_Shmem___ipc____Multipart)
{
    RunMultiThreadedMultipart("shmem", "ipc://test_MT_Shmem___ipc____Multipart");
}

#ifdef BUILD_NANOMSG_TRANSPORT
TEST(PushPull, MT_Nanomsg_ipc____Multipart)
{
    RunMultiThreadedMultipart("nanomsg", "ipc://test_MT_Nanomsg_ipc____Multipart");
}
#endif /* BUILD_NANOMSG_TRANSPORT */

} // namespace
