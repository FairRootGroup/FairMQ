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
#include <fairmq/ProgOptions.h>

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

    fair::mq::ProgOptions config;
    config.SetProperty<string>("session", std::to_string(session));
    auto factory = FairMQTransportFactory::CreateTransportFactory(transport, fair::mq::tools::Uuid(), &config);
    FairMQTransportFactory* factoryptr = factory.get();
    FairMQChannel push("Push", "push", factory);
    ASSERT_TRUE(push.Bind(address));
    FairMQChannel pull("Pull", "pull", factory);
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

    fair::mq::ProgOptions config;
    config.SetProperty<string>("session", std::to_string(session));
    config.SetProperty<int>("io-threads", 1);
    config.SetProperty<size_t>("shm-segment-size", 20000000);
    auto factory = FairMQTransportFactory::CreateTransportFactory(transport, fair::mq::tools::Uuid(), &config);
    FairMQChannel push("Push", "push", factory);
    ASSERT_TRUE(push.Bind(address));
    FairMQChannel pull("Pull", "pull", factory);
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

TEST(PushPull, Multipart_ST_inproc_zeromq)
{
    RunSingleThreadedMultipart("zeromq", "inproc://test");
}

TEST(PushPull, Multipart_ST_inproc_shmem)
{
    RunSingleThreadedMultipart("shmem", "inproc://test");
}

#ifdef BUILD_NANOMSG_TRANSPORT
TEST(PushPull, Multipart_ST_inproc_nanomsg)
{
    RunSingleThreadedMultipart("nanomsg", "inproc://test");
}
#endif /* BUILD_NANOMSG_TRANSPORT */

TEST(PushPull, Multipart_ST_ipc_zeromq)
{
    RunSingleThreadedMultipart("zeromq", "ipc://test_Multipart_ST_ipc_zeromq");
}

TEST(PushPull, Multipart_ST_ipc_shmen)
{
    RunSingleThreadedMultipart("shmem", "ipc://test_Multipart_ST_ipc_shmen");
}

#ifdef BUILD_NANOMSG_TRANSPORT
TEST(PushPull, Multipart_ST_ipc_nanomsg)
{
    RunSingleThreadedMultipart("nanomsg", "ipc://test_Multipart_ST_ipc_nanomsg");
}
#endif /* BUILD_NANOMSG_TRANSPORT */

TEST(PushPull, Multipart_MT_inproc_zeromq)
{
    RunMultiThreadedMultipart("zeromq", "inproc://test");
}

TEST(PushPull, Multipart_MT_inproc_shmem)
{
    RunMultiThreadedMultipart("shmem", "inproc://test");
}

#ifdef BUILD_NANOMSG_TRANSPORT
TEST(PushPull, Multipart_MT_inproc_nanomsg)
{
    RunMultiThreadedMultipart("nanomsg", "inproc://test");
}
#endif /* BUILD_NANOMSG_TRANSPORT */

TEST(PushPull, Multipart_MT_ipc_zeromq)
{
    RunMultiThreadedMultipart("zeromq", "ipc://test_Multipart_MT_ipc_zeromq");
}

TEST(PushPull, Multipart_MT_ipc_shmem)
{
    RunMultiThreadedMultipart("shmem", "ipc://test_Multipart_MT_ipc_shmem");
}

#ifdef BUILD_NANOMSG_TRANSPORT
TEST(PushPull, Multipart_MT_ipc_nanomsg)
{
    RunMultiThreadedMultipart("nanomsg", "ipc://test_Multipart_MT_ipc_nanomsg");
}
#endif /* BUILD_NANOMSG_TRANSPORT */

} // namespace
