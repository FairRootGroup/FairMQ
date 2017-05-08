/********************************************************************************
 *    Copyright (C) 2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <gtest/gtest.h>
#include <FairMQParts.h>
#include <FairMQDevice.h>
#include <zeromq/FairMQTransportFactoryZMQ.h>
#include <string>
#include <sstream>
#include <algorithm>

namespace
{

using namespace std;

class RandomAccessIterator : public ::testing::Test {
  protected:
    FairMQParts mParts;
    FairMQTransportFactoryZMQ mFactory;
    const string mS1, mS2, mS3;

    RandomAccessIterator()
    : mParts(FairMQParts{}),
      mFactory(FairMQTransportFactoryZMQ{}),
      mS1("1"),
      mS2("2"),
      mS3("3")
    {

        mParts.AddPart(mFactory.CreateMessage(const_cast<char*>(mS1.c_str()),
            mS1.length(), FairMQDevice::FairMQSimpleMsgCleanup<string>));
        mParts.AddPart(mFactory.CreateMessage(const_cast<char*>(mS2.c_str()),
            mS2.length(), FairMQDevice::FairMQSimpleMsgCleanup<string>));
        mParts.AddPart(mFactory.CreateMessage(const_cast<char*>(mS3.c_str()),
            mS3.length(), FairMQDevice::FairMQSimpleMsgCleanup<string>));
    }
};

TEST_F(RandomAccessIterator, RangeForLoopConst)
{
    stringstream out;
    for (const auto& part : mParts) {
        out << string{static_cast<char*>(part->GetData()), part->GetSize()};
    }

    ASSERT_EQ(out.str(), "123");
}

TEST_F(RandomAccessIterator, RangeForLoopMutation)
{
    auto s = string{"4"};

    for (auto&& part : mParts) {
        part = mFactory.CreateMessage(const_cast<char*>(s.c_str()),
            s.length(), FairMQDevice::FairMQSimpleMsgCleanup<string>);
    }

    stringstream out;
    for (const auto& part : mParts) {
        out << string{static_cast<char*>(part->GetData()), part->GetSize()};
    }

    ASSERT_EQ(out.str(), "444");
}

TEST_F(RandomAccessIterator, ForEachConst)
{
    stringstream out;
    for_each(mParts.cbegin(), mParts.cend(), [&out](const FairMQMessagePtr& part) {
        out << string{static_cast<char*>(part->GetData()), part->GetSize()};
    });

    ASSERT_EQ(out.str(), "123");
}

} // namespace
