/********************************************************************************
 *    Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <fairmq/Parts.h>
#include <fairmq/TransportFactory.h>
#include <gtest/gtest.h>
#include <memory>

namespace {

using namespace std;

class AddPart : public ::testing::Test
{
  protected:
    fair::mq::Parts mParts;
    shared_ptr<fair::mq::TransportFactory> mFactory;

    AddPart()
        : mFactory(fair::mq::TransportFactory::CreateTransportFactory("zeromq"))
    {
    }
};

TEST_F(AddPart, AnotherParts)
{
    mParts.AddPart(mFactory->NewSimpleMessage("1"));
    mParts.AddPart(mFactory->NewSimpleMessage("2"));
    mParts.AddPart(mFactory->NewSimpleMessage("3"));
    auto const oldSize = mParts.Size();

    fair::mq::Parts anotherParts;
    anotherParts.AddPart(mFactory->NewSimpleMessage("4"));
    anotherParts.AddPart(mFactory->NewSimpleMessage("5"));
    anotherParts.AddPart(mFactory->NewSimpleMessage("6"));
    auto const addedSize = anotherParts.Size();
    auto const newSize = oldSize + addedSize;

    mParts.AddPart(std::move(anotherParts));
    ASSERT_TRUE(mParts.Size() == newSize);
}

TEST_F(AddPart, SinglePart)
{
    auto const oldSize = mParts.Size();
    mParts.AddPart(mFactory->NewSimpleMessage("asdf"));
    ASSERT_TRUE(mParts.Size() == oldSize + 1);
}

TEST_F(AddPart, MultipleParts)
{
    auto const oldSize = mParts.Size();
    mParts.AddPart(mFactory->NewSimpleMessage("1"),
                   mFactory->NewSimpleMessage("2"),
                   mFactory->NewSimpleMessage("3"));
    ASSERT_TRUE(mParts.Size() == oldSize + 3);
}

TEST(Construction, AppendMultipleParts)
{
    auto factory = fair::mq::TransportFactory::CreateTransportFactory("zeromq");
    fair::mq::Parts newParts(factory->NewSimpleMessage("1"),
                             factory->NewSimpleMessage("2"),
                             factory->NewSimpleMessage("3"));
    ASSERT_TRUE(newParts.Size() == 3);
}

}   // namespace
