/********************************************************************************
 *    Copyright (C) 2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <gtest/gtest.h>
#include <fairmq/Parts.h>
#include <fairmq/TransportFactory.h>
#include <string>
#include <sstream>
#include <algorithm>
#include <memory>

namespace
{

using namespace std;

class RandomAccessIterator : public ::testing::Test {
  protected:
    fair::mq::Parts mParts;
    shared_ptr<fair::mq::TransportFactory> mFactory;

    RandomAccessIterator()
        : mFactory(fair::mq::TransportFactory::CreateTransportFactory("zeromq"))
    {
        mParts.AddPart(mFactory->NewSimpleMessage("1"));
        mParts.AddPart(mFactory->NewSimpleMessage("2"));
        mParts.AddPart(mFactory->NewSimpleMessage("3"));
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
        part = mFactory->NewSimpleMessage(s);
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
    for_each(mParts.cbegin(), mParts.cend(), [&out](const fair::mq::MessagePtr& part) {
        out << string{static_cast<char*>(part->GetData()), part->GetSize()};
    });

    ASSERT_EQ(out.str(), "123");
}

} // namespace
