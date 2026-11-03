/********************************************************************************
 *    Copyright (C) 2018 Goethe University Frankfurt                            *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <fairmq/TransportFactory.h>
#include <fairmq/MemoryResourceTools.h>
#include <fairmq/ProgOptions.h>
#include <fairmq/tools/Unique.h>

#include <boost/container/pmr/polymorphic_allocator.hpp>

#include <fairmq/Channel.h>

#include <helper/Transport.h>
#include <helper/TransportParam.h>
#include <gtest/gtest.h>

#include <cstddef>
#include <cstring>
#include <string>
#include <vector>

namespace
{

using namespace std;
using namespace fair::mq;

using FactoryType = shared_ptr<TransportFactory>;

struct TestData
{
    int i{1};
    static int nallocated;
    static int nallocations;
    static int ndeallocations;

    TestData()
    {
        ++nallocated;
        ++nallocations;
    }

    TestData& operator=(const TestData&) = delete;
    TestData& operator=(TestData&&) = delete;

    TestData(const TestData& in)
        : i{in.i}
    {
        ++nallocated;
        ++nallocations;
    }

    TestData(const TestData&& in)
        : i{in.i}
    {
        ++nallocated;
        ++nallocations;
    }

    TestData(int in)
        : i{in}
    {
        ++nallocated;
        ++nallocations;
    }

    ~TestData()
    {
        --nallocated;
        ++ndeallocations;
    }
};

int TestData::nallocated = 0;
int TestData::nallocations = 0;
int TestData::ndeallocations = 0;

TEST(MemoryResources, transportAllocatorMap)
{
    ProgOptions config;
    test::Configure(config);

    FactoryType factoryZMQ = TransportFactory::CreateTransportFactory("zeromq", fair::mq::tools::Uuid(), &config);
    FactoryType factorySHM = TransportFactory::CreateTransportFactory("shmem", fair::mq::tools::Uuid(), &config);

    auto allocZMQ = factoryZMQ->GetMemoryResource();
    auto allocSHM = factorySHM->GetMemoryResource();

    EXPECT_NE(allocZMQ, nullptr);
    EXPECT_NE(allocSHM, allocZMQ);
    auto _tmp = factoryZMQ->GetMemoryResource();
    EXPECT_EQ(_tmp, allocZMQ);
}

using namespace fair::mq::pmr;

TEST(MemoryResources, allocator)
{
    TestData::nallocations = 0;
    TestData::ndeallocations = 0;

    ProgOptions config;
    test::Configure(config);

    FactoryType factoryZMQ = TransportFactory::CreateTransportFactory("zeromq", fair::mq::tools::Uuid(), &config);

    auto allocZMQ = factoryZMQ->GetMemoryResource();

    {
        std::vector<TestData, polymorphic_allocator<TestData>> v(polymorphic_allocator<TestData>{allocZMQ});
        v.reserve(3);
        EXPECT_EQ(v.capacity(), 3U);
        EXPECT_EQ(allocZMQ->getNumberOfMessages(), 1U);
        v.emplace_back(1);
        v.emplace_back(2);
        v.emplace_back(3);
        EXPECT_EQ(reinterpret_cast<fair::mq::byte*>(v.data() + v.size()) - reinterpret_cast<fair::mq::byte*>(v.data()), static_cast<std::ptrdiff_t>(3 * sizeof(TestData)));
        EXPECT_EQ(TestData::nallocated, 3);
    }
    EXPECT_EQ(TestData::nallocated, 0);
    EXPECT_EQ(TestData::nallocations, TestData::ndeallocations);
}

TEST(MemoryResources, getMessage)
{
    TestData::nallocations = 0;
    TestData::ndeallocations = 0;

    ProgOptions config;
    test::Configure(config);

    FactoryType factoryZMQ = TransportFactory::CreateTransportFactory("zeromq", fair::mq::tools::Uuid(), &config);
    FactoryType factorySHM = TransportFactory::CreateTransportFactory("shmem", fair::mq::tools::Uuid(), &config);

    auto allocZMQ = factoryZMQ->GetMemoryResource();

    MessagePtr message{nullptr};

    int* messageArray{nullptr};

    // test message creation on the same channel it was allocated with
    {
        std::vector<TestData, polymorphic_allocator<TestData>> v(polymorphic_allocator<TestData>{allocZMQ});
        v.emplace_back(1);
        v.emplace_back(2);
        v.emplace_back(3);
        void* vectorBeginPtr = &v[0];
        message = getMessage(std::move(v));
        ASSERT_NE(message, nullptr);
        EXPECT_EQ(message->GetData(), vectorBeginPtr);
    }
    EXPECT_EQ(message->GetSize(), 3 * sizeof(TestData));
    messageArray = static_cast<int*>(message->GetData());
    EXPECT_EQ(messageArray[0], 1);
    EXPECT_EQ(messageArray[1], 2);
    EXPECT_EQ(messageArray[2], 3);

    // test message creation on a different channel than it was allocated with
    {
        std::vector<TestData, polymorphic_allocator<TestData>> v(polymorphic_allocator<TestData>{allocZMQ});
        v.emplace_back(4);
        v.emplace_back(5);
        v.emplace_back(6);
        void* vectorBeginPtr = &v[0];
        message = getMessage(std::move(v), *factorySHM);
        ASSERT_NE(message, nullptr);
        EXPECT_NE(message->GetData(), vectorBeginPtr);
    }

    EXPECT_EQ(message->GetSize(), 3 * sizeof(TestData));
    messageArray = static_cast<int*>(message->GetData());
    EXPECT_EQ(messageArray[0], 4);
    EXPECT_EQ(messageArray[1], 5);
    EXPECT_EQ(messageArray[2], 6);
}

using PmrMessage = test::TransportTest;

// A container that uses the memory resource of a transport becomes a message
// without a copy, and the message transfers the container data.
TEST_P(PmrMessage, FromContainerWithoutCopy)
{
    ProgOptions config;
    test::Configure(config, tools::Uuid(), GetParam().expandedShmMetadata);
    auto factory = TransportFactory::CreateTransportFactory(GetParam().transport, tools::Uuid(), &config);

    std::vector<int, polymorphic_allocator<int>> v(polymorphic_allocator<int>{factory->GetMemoryResource()});
    v.reserve(4);
    v.push_back(7);
    v.push_back(8);
    v.push_back(9);
    void const* data = v.data();

    auto message = getMessage(std::move(v));
    ASSERT_NE(message, nullptr);
    EXPECT_EQ(message->GetData(), data);
    EXPECT_EQ(message->GetSize(), 3 * sizeof(int));
    EXPECT_EQ(factory->GetMemoryResource()->getNumberOfMessages(), 0U);

    Channel push("push", "push", factory);
    Channel pull("pull", "pull", factory);
    test::IpcAddress const address("pmr_message");
    ASSERT_TRUE(pull.Bind(address));
    ASSERT_TRUE(push.Connect(address));
    ASSERT_EQ(push.Send(message, 5000), static_cast<int64_t>(3 * sizeof(int)));

    auto in(pull.NewMessage());
    ASSERT_EQ(pull.Receive(in, 5000), static_cast<int64_t>(3 * sizeof(int)));
    auto const* received = static_cast<int const*>(in->GetData());
    EXPECT_EQ(received[0], 7);
    EXPECT_EQ(received[1], 8);
    EXPECT_EQ(received[2], 9);
}

INSTANTIATE_TEST_SUITE_P(, PmrMessage, test::AllTransports(), test::TransportParamName());

}   // namespace
