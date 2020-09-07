/********************************************************************************
 *    Copyright (C) 2018 Goethe University Frankfurt                            *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <cstring>
#include <fairmq/FairMQTransportFactory.h>
#include <fairmq/MemoryResourceTools.h>
#include <boost/container/pmr/polymorphic_allocator.hpp>
#include <gtest/gtest.h>
#include <vector>

namespace {

using namespace std;
using namespace fair::mq;
using factoryType = std::shared_ptr<FairMQTransportFactory>;

factoryType factoryZMQ = FairMQTransportFactory::CreateTransportFactory("zeromq");
factoryType factorySHM = FairMQTransportFactory::CreateTransportFactory("shmem");

struct testData
{
    int i{1};
    static int nallocated;
    static int nallocations;
    static int ndeallocations;
    testData()
    {
        ++nallocated;
        ++nallocations;
    }
    testData(const testData& in)
        : i{in.i}
    {
        ++nallocated;
        ++nallocations;
    }
    testData(const testData&& in)
        : i{in.i}
    {
        ++nallocated;
        ++nallocations;
    }
    testData(int in)
        : i{in}
    {
        ++nallocated;
        ++nallocations;
    }
    ~testData()
    {
        --nallocated;
        ++ndeallocations;
    }
};

int testData::nallocated = 0;
int testData::nallocations = 0;
int testData::ndeallocations = 0;

auto allocZMQ = factoryZMQ -> GetMemoryResource();
auto allocSHM = factorySHM -> GetMemoryResource();

TEST(MemoryResources, transportallocatormap)
{
    EXPECT_TRUE(allocZMQ != nullptr && allocSHM != allocZMQ);
    auto _tmp = factoryZMQ->GetMemoryResource();
    EXPECT_TRUE(_tmp == allocZMQ);
}

using namespace fair::mq::pmr;

TEST(MemoryResources, allocator)
{
    testData::nallocations = 0;
    testData::ndeallocations = 0;

    {
        std::vector<testData, polymorphic_allocator<testData>> v(polymorphic_allocator<testData>{allocZMQ});
        v.reserve(3);
        EXPECT_TRUE(v.capacity() == 3);
        EXPECT_TRUE(allocZMQ->getNumberOfMessages() == 1);
        v.emplace_back(1);
        v.emplace_back(2);
        v.emplace_back(3);
        EXPECT_TRUE((fair::mq::byte*)&(*v.end()) - (fair::mq::byte*)&(*v.begin()) == 3 * sizeof(testData));
        EXPECT_TRUE(testData::nallocated == 3);
    }
    EXPECT_TRUE(testData::nallocated == 0);
    EXPECT_TRUE(testData::nallocations == testData::ndeallocations);
}

TEST(MemoryResources, getMessage)
{
    testData::nallocations = 0;
    testData::ndeallocations = 0;

    FairMQMessagePtr message{nullptr};

    int* messageArray{nullptr};

    // test message creation on the same channel it was allocated with
    {
        std::vector<testData, polymorphic_allocator<testData>> v(polymorphic_allocator<testData>{allocZMQ});
        v.emplace_back(1);
        v.emplace_back(2);
        v.emplace_back(3);
        void* vectorBeginPtr = &v[0];
        message = getMessage(std::move(v));
        EXPECT_TRUE(message != nullptr);
        EXPECT_TRUE(message->GetData() == vectorBeginPtr);
    }
    EXPECT_TRUE(message->GetSize() == 3 * sizeof(testData));
    messageArray = static_cast<int*>(message->GetData());
    EXPECT_TRUE(messageArray[0] == 1 && messageArray[1] == 2 && messageArray[2] == 3);

    // test message creation on a different channel than it was allocated with
    {
        std::vector<testData, polymorphic_allocator<testData>> v(polymorphic_allocator<testData>{allocZMQ});
        v.emplace_back(4);
        v.emplace_back(5);
        v.emplace_back(6);
        void* vectorBeginPtr = &v[0];
        message = getMessage(std::move(v), *factorySHM);
        EXPECT_TRUE(message != nullptr);
        EXPECT_TRUE(message->GetData() != vectorBeginPtr);
    }

    EXPECT_TRUE(message->GetSize() == 3 * sizeof(testData));
    messageArray = static_cast<int*>(message->GetData());
    EXPECT_TRUE(messageArray[0] == 4 && messageArray[1] == 5 && messageArray[2] == 6);
}

}   // namespace
