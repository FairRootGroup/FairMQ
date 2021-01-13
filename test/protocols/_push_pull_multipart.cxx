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
#include <fairmq/tools/Unique.h>
#include <fairmq/ProgOptions.h>

#include <algorithm>
#include <memory>
#include <sstream>
#include <string>
#include <thread>

namespace
{

using namespace std;

auto RunSingleThreadedMultipart(string transport, string address1, string address2) -> void {

    size_t session{fair::mq::tools::UuidHash()};

    fair::mq::ProgOptions config;
    config.SetProperty<string>("session", std::to_string(session));

    auto factory = FairMQTransportFactory::CreateTransportFactory(transport, fair::mq::tools::Uuid(), &config);

    FairMQChannel push1("Push1", "push", factory);
    ASSERT_TRUE(push1.Bind(address1));
    FairMQChannel pull1("Pull1", "pull", factory);
    ASSERT_TRUE(pull1.Connect(address1));
    FairMQChannel push2("Push2", "push", factory);
    ASSERT_TRUE(push2.Bind(address2));
    FairMQChannel pull2("Pull2", "pull", factory);
    ASSERT_TRUE(pull2.Connect(address2));

    // TODO validate that fTransportFactory is not nullptr
    // TODO validate that fSocket is not nullptr
    ASSERT_TRUE(push1.Validate());
    ASSERT_TRUE(pull1.Validate());
    ASSERT_TRUE(push2.Validate());
    ASSERT_TRUE(pull2.Validate());

    {
        FairMQParts multiplePartsOut;
        multiplePartsOut.AddPart(push1.NewSimpleMessage("1"));
        multiplePartsOut.AddPart(push1.NewSimpleMessage("2"));
        multiplePartsOut.AddPart(push1.NewSimpleMessage("3"));
        ASSERT_GE(push1.Send(multiplePartsOut), 0);

        FairMQParts singlePartOut;
        singlePartOut.AddPart(push1.NewSimpleMessage("4"));
        ASSERT_GE(push1.Send(singlePartOut), 0);
    }

    FairMQParts multipleParts;
    ASSERT_GE(pull1.Receive(multipleParts), 0);

    stringstream multiple;
    for_each(multipleParts.cbegin(), multipleParts.cend(), [&multiple, &factory](const FairMQMessagePtr& part) {
        multiple << string{static_cast<char*>(part->GetData()), part->GetSize()};
        ASSERT_EQ(part->GetTransport(), factory.get());
    });
    ASSERT_EQ(multiple.str(), "123");

    FairMQParts singlePart;
    ASSERT_GE(pull1.Receive(singlePart), 0);

    stringstream single;
    for_each(singlePart.cbegin(), singlePart.cend(), [&single](const FairMQMessagePtr& part) {
        single << string{static_cast<char*>(part->GetData()), part->GetSize()};
    });
    ASSERT_EQ(single.str(), "4");

    ASSERT_GE(push2.Send(singlePart), 0);
    ASSERT_GE(push2.Send(multipleParts), 0);

    {
        FairMQParts singlePartIn;
        ASSERT_GE(pull2.Receive(singlePartIn), 0);
        stringstream singleIn;
        for_each(singlePartIn.cbegin(), singlePartIn.cend(), [&singleIn](const FairMQMessagePtr& part) {
            singleIn << string{static_cast<char*>(part->GetData()), part->GetSize()};
        });
        ASSERT_EQ(singleIn.str(), "4");

        FairMQParts multiplePartsIn;
        ASSERT_GE(pull2.Receive(multiplePartsIn), 0);
        stringstream multipleIn;
        for_each(multiplePartsIn.cbegin(), multiplePartsIn.cend(), [&multipleIn, &factory](const FairMQMessagePtr& part) {
            multipleIn << string{static_cast<char*>(part->GetData()), part->GetSize()};
            ASSERT_EQ(part->GetTransport(), factory.get());
        });
        ASSERT_EQ(multipleIn.str(), "123");
    }
}

auto RunMultiThreadedMultipart(string transport, string address1) -> void
{
    size_t session{fair::mq::tools::UuidHash()};

    fair::mq::ProgOptions config;
    config.SetProperty<string>("session", std::to_string(session));
    config.SetProperty<int>("io-threads", 1);
    config.SetProperty<size_t>("shm-segment-size", 20000000);

    auto factory = FairMQTransportFactory::CreateTransportFactory(transport, fair::mq::tools::Uuid(), &config);

    FairMQChannel push1("Push1", "push", factory);
    ASSERT_TRUE(push1.Bind(address1));
    FairMQChannel pull1("Pull1", "pull", factory);
    ASSERT_TRUE(pull1.Connect(address1));

    auto pusher = thread{[&push1](){
        ASSERT_TRUE(push1.Validate());

        FairMQParts sent;
        sent.AddPart(push1.NewSimpleMessage("1"));
        sent.AddPart(push1.NewSimpleMessage("2"));
        sent.AddPart(push1.NewSimpleMessage("3"));

        ASSERT_GE(push1.Send(sent), 0);
    }};

    auto puller = thread{[&pull1](){
        ASSERT_TRUE(pull1.Validate());

        FairMQParts received;
        ASSERT_GE(pull1.Receive(received), 0);

        stringstream out;
        for_each(received.cbegin(), received.cend(), [&out](const FairMQMessagePtr& part) {
            out << string{static_cast<char*>(part->GetData()), part->GetSize()};
        });
        ASSERT_EQ(out.str(), "123");
    }};

    pusher.join();
    puller.join();
}

TEST(PushPull, Multipart_ST_inproc_zeromq)
{
    RunSingleThreadedMultipart("zeromq", "inproc://test1", "inproc://test2");
}

TEST(PushPull, Multipart_ST_inproc_shmem)
{
    RunSingleThreadedMultipart("shmem", "inproc://test1", "inproc://test2");
}

TEST(PushPull, Multipart_ST_ipc_zeromq)
{
    RunSingleThreadedMultipart("zeromq", "ipc://test_Multipart_ST_ipc_zeromq_1", "ipc://test_Multipart_ST_ipc_zeromq_2");
}

TEST(PushPull, Multipart_ST_ipc_shmem)
{
    RunSingleThreadedMultipart("shmem", "ipc://test_Multipart_ST_ipc_shmem_1", "ipc://test_Multipart_ST_ipc_shmem_2");
}

TEST(PushPull, Multipart_MT_inproc_zeromq)
{
    RunMultiThreadedMultipart("zeromq", "inproc://test_1");
}

TEST(PushPull, Multipart_MT_inproc_shmem)
{
    RunMultiThreadedMultipart("shmem", "inproc://test_1");
}

TEST(PushPull, Multipart_MT_ipc_zeromq)
{
    RunMultiThreadedMultipart("zeromq", "ipc://test_Multipart_MT_ipc_zeromq_1");
}

TEST(PushPull, Multipart_MT_ipc_shmem)
{
    RunMultiThreadedMultipart("shmem", "ipc://test_Multipart_MT_ipc_shmem_1");
}

} // namespace
