/********************************************************************************
 * Copyright (C) 2017-2021 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <algorithm>
#include <fairlogger/Logger.h>
#include <fairmq/Channel.h>
#include <fairmq/Parts.h>
#include <fairmq/ProgOptions.h>
#include <fairmq/TransportFactory.h>
#include <fairmq/tools/Unique.h>
#include <helper/Transport.h>
#include <gtest/gtest.h>
#include <memory>
#include <sstream>
#include <string>
#include <thread>

namespace
{

using namespace std;
using namespace fair::mq;

auto RunSingleThreadedMultipart(string const& transport, string const& address1, string const& address2, bool expandedShmMetadata) -> void
{

    fair::mq::ProgOptions config;
    test::Configure(config, tools::Uuid(), expandedShmMetadata);


    auto factory = TransportFactory::CreateTransportFactory(transport, tools::Uuid(), &config);

    Channel push1("Push1", "push", factory);
    ASSERT_TRUE(push1.Bind(address1));
    Channel pull1("Pull1", "pull", factory);
    ASSERT_TRUE(pull1.Connect(address1));
    Channel push2("Push2", "push", factory);
    ASSERT_TRUE(push2.Bind(address2));
    Channel pull2("Pull2", "pull", factory);
    ASSERT_TRUE(pull2.Connect(address2));

    // TODO validate that fTransportFactory is not nullptr
    // TODO validate that fSocket is not nullptr
    ASSERT_TRUE(push1.Validate());
    ASSERT_TRUE(pull1.Validate());
    ASSERT_TRUE(push2.Validate());
    ASSERT_TRUE(pull2.Validate());

    {
        Parts multiplePartsOut;
        multiplePartsOut.AddPart(push1.NewSimpleMessage("1"));
        multiplePartsOut.AddPart(push1.NewSimpleMessage("2"));
        multiplePartsOut.AddPart(push1.NewSimpleMessage("3"));
        ASSERT_GE(push1.Send(multiplePartsOut), 0);

        Parts singlePartOut;
        singlePartOut.AddPart(push1.NewSimpleMessage("4"));
        ASSERT_GE(push1.Send(singlePartOut), 0);
    }

    Parts multipleParts;
    ASSERT_GE(pull1.Receive(multipleParts), 0);

    stringstream multiple;
    for_each(multipleParts.cbegin(), multipleParts.cend(), [&multiple, &factory](const MessagePtr& part) {
        multiple << string{static_cast<char*>(part->GetData()), part->GetSize()};
        ASSERT_EQ(part->GetTransport(), factory.get());
    });
    ASSERT_EQ(multiple.str(), "123");

    Parts singlePart;
    ASSERT_GE(pull1.Receive(singlePart), 0);

    stringstream single;
    for_each(singlePart.cbegin(), singlePart.cend(), [&single](const MessagePtr& part) {
        single << string{static_cast<char*>(part->GetData()), part->GetSize()};
    });
    ASSERT_EQ(single.str(), "4");

    ASSERT_GE(push2.Send(singlePart), 0);
    ASSERT_GE(push2.Send(multipleParts), 0);

    {
        Parts singlePartIn;
        ASSERT_GE(pull2.Receive(singlePartIn), 0);
        stringstream singleIn;
        for_each(singlePartIn.cbegin(), singlePartIn.cend(), [&singleIn](const MessagePtr& part) {
            singleIn << string{static_cast<char*>(part->GetData()), part->GetSize()};
        });
        ASSERT_EQ(singleIn.str(), "4");

        Parts multiplePartsIn;
        ASSERT_GE(pull2.Receive(multiplePartsIn), 0);
        stringstream multipleIn;
        for_each(multiplePartsIn.cbegin(), multiplePartsIn.cend(), [&multipleIn, &factory](const MessagePtr& part) {
            multipleIn << string{static_cast<char*>(part->GetData()), part->GetSize()};
            ASSERT_EQ(part->GetTransport(), factory.get());
        });
        ASSERT_EQ(multipleIn.str(), "123");
    }
}

auto RunMultiThreadedMultipart(string const& transport, string const& address1, bool expandedShmMetadata) -> void
{
    ProgOptions config;
    test::Configure(config, tools::Uuid(), expandedShmMetadata);
    config.SetProperty<int>("io-threads", 1);
    config.SetProperty<size_t>("shm-segment-size", 20000000); // NOLINT


    auto factory = TransportFactory::CreateTransportFactory(transport, tools::Uuid(), &config);

    Channel push1("Push1", "push", factory);
    ASSERT_TRUE(push1.Bind(address1));
    Channel pull1("Pull1", "pull", factory);
    ASSERT_TRUE(pull1.Connect(address1));

    auto pusher = thread{[&push1](){
        ASSERT_TRUE(push1.Validate());

        Parts sent;
        sent.AddPart(push1.NewSimpleMessage("1"));
        sent.AddPart(push1.NewSimpleMessage("2"));
        sent.AddPart(push1.NewSimpleMessage("3"));

        ASSERT_GE(push1.Send(sent), 0);
    }};

    auto puller = thread{[&pull1](){
        ASSERT_TRUE(pull1.Validate());

        Parts received;
        ASSERT_GE(pull1.Receive(received), 0);

        stringstream out;
        for_each(received.cbegin(), received.cend(), [&out](const MessagePtr& part) {
            out << string{static_cast<char*>(part->GetData()), part->GetSize()};
        });
        ASSERT_EQ(out.str(), "123");
    }};

    pusher.join();
    puller.join();
}

TEST(PushPull, Multipart_SingleThreaded_inproc_zeromq) // NOLINT
{
    RunSingleThreadedMultipart("zeromq", "inproc://test1", "inproc://test2", false);
}

TEST(PushPull, Multipart_SingleThreaded_inproc_shmem) // NOLINT
{
    RunSingleThreadedMultipart("shmem", "inproc://test1", "inproc://test2", false);
}

TEST(PushPull, Multipart_SingleThreaded_inproc_shmem_expanded_metadata) // NOLINT
{
    RunSingleThreadedMultipart("shmem", "inproc://test1", "inproc://test2", true);
}

TEST(PushPull, Multipart_SingleThreaded_ipc_zeromq) // NOLINT
{
    test::IpcAddress const address1("multipart_1"), address2("multipart_2");
    RunSingleThreadedMultipart("zeromq", address1, address2, false);
}

TEST(PushPull, Multipart_SingleThreaded_ipc_shmem) // NOLINT
{
    test::IpcAddress const address1("multipart_1"), address2("multipart_2");
    RunSingleThreadedMultipart("shmem", address1, address2, false);
}

TEST(PushPull, Multipart_SingleThreaded_ipc_shmem_expanded_metadata) // NOLINT
{
    test::IpcAddress const address1("multipart_1"), address2("multipart_2");
    RunSingleThreadedMultipart("shmem", address1, address2, true);
}

TEST(PushPull, Multipart_MultiThreaded_inproc_zeromq) // NOLINT
{
    RunMultiThreadedMultipart("zeromq", "inproc://test_1", false);
}

TEST(PushPull, Multipart_MultiThreaded_inproc_shmem) // NOLINT
{
    RunMultiThreadedMultipart("shmem", "inproc://test_1", false);
}

TEST(PushPull, Multipart_MultiThreaded_inproc_shmem_expanded_metadata) // NOLINT
{
    RunMultiThreadedMultipart("shmem", "inproc://test_1", true);
}

TEST(PushPull, Multipart_MultiThreaded_ipc_zeromq) // NOLINT
{
    test::IpcAddress const address("multipart");
    RunMultiThreadedMultipart("zeromq", address, false);
}

TEST(PushPull, Multipart_MultiThreaded_ipc_shmem) // NOLINT
{
    test::IpcAddress const address("multipart");
    RunMultiThreadedMultipart("shmem", address, false);
}

TEST(PushPull, Multipart_MultiThreaded_ipc_shmem_expanded_metadata) // NOLINT
{
    test::IpcAddress const address("multipart");
    RunMultiThreadedMultipart("shmem", address, true);
}

} // namespace
