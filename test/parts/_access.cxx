/********************************************************************************
 *    Copyright (C) 2026 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <fairmq/Channel.h>
#include <fairmq/Parts.h>
#include <fairmq/ProgOptions.h>
#include <fairmq/TransportFactory.h>
#include <fairmq/tools/Unique.h>

#include <helper/Transport.h>
#include <helper/TransportParam.h>

#include <gtest/gtest.h>

#include <stdexcept>
#include <string>

namespace
{

using namespace std;
using namespace fair::mq;

auto AsString(Message const& msg) -> string
{
    return {static_cast<char const*>(msg.GetData()), msg.GetSize()};
}

class PartsAccess : public testing::Test
{
  protected:
    PartsAccess()
    {
        test::Configure(fConfig);
        fFactory = TransportFactory::CreateTransportFactory("zeromq", tools::Uuid(), &fConfig);
    }

    ProgOptions fConfig;
    shared_ptr<TransportFactory> fFactory;
};

TEST_F(PartsAccess, VariadicConstructor)
{
    Parts parts(fFactory->NewSimpleMessage("a"), fFactory->NewSimpleMessage("b"), fFactory->NewSimpleMessage("c"));
    ASSERT_EQ(parts.Size(), 3U);
    EXPECT_EQ(AsString(parts[0]), "a");
    EXPECT_EQ(AsString(parts[1]), "b");
    EXPECT_EQ(AsString(parts[2]), "c");
}

TEST_F(PartsAccess, AtAndIndex)
{
    Parts parts(fFactory->NewSimpleMessage("a"), fFactory->NewSimpleMessage("b"));
    Parts const& constParts = parts;

    EXPECT_EQ(AsString(*parts.At(1)), "b");
    EXPECT_EQ(AsString(*constParts.At(0)), "a");
    EXPECT_EQ(&parts[1], parts.At(1).get());
    EXPECT_EQ(&constParts[0], constParts.At(0).get());
    EXPECT_THROW(parts.At(2), out_of_range);
    EXPECT_THROW(constParts.At(2), out_of_range);

    // At() returns a reference to the owning pointer: the part can be replaced.
    parts.At(0) = fFactory->NewSimpleMessage("z");
    EXPECT_EQ(AsString(parts[0]), "z");
}

TEST_F(PartsAccess, EmptyAndClear)
{
    Parts parts;
    EXPECT_TRUE(parts.Empty());
    EXPECT_EQ(parts.Size(), 0U);
    EXPECT_EQ(parts.begin(), parts.end());

    parts.AddPart(fFactory->NewSimpleMessage("a"), fFactory->NewSimpleMessage("b"));
    EXPECT_FALSE(parts.Empty());

    parts.Clear();
    EXPECT_TRUE(parts.Empty());
    EXPECT_EQ(parts.Size(), 0U);
}

TEST_F(PartsAccess, MoveLeavesTheSourceEmpty)
{
    Parts source(fFactory->NewSimpleMessage("a"));
    Parts target(std::move(source));
    EXPECT_EQ(target.Size(), 1U);
    EXPECT_TRUE(source.Empty()); // NOLINT(bugprone-use-after-move)
}

using PartsTransfer = test::TransportTest;

TEST_P(PartsTransfer, EmptyAndNonEmpty)
{
    ProgOptions config;
    test::Configure(config, tools::Uuid(), GetParam().expandedShmMetadata);
    auto factory = TransportFactory::CreateTransportFactory(GetParam().transport, tools::Uuid(), &config);

    Channel push("push", "push", factory);
    Channel pull("pull", "pull", factory);
    test::IpcAddress const address("parts_transfer");
    ASSERT_TRUE(pull.Bind(address));
    ASSERT_TRUE(push.Connect(address));

    Parts out(push.NewSimpleMessage("ab"), push.NewMessage(), push.NewSimpleMessage("cde"));
    ASSERT_EQ(push.Send(out, 5000), 5);

    Parts in;
    ASSERT_EQ(pull.Receive(in, 5000), 5);
    ASSERT_EQ(in.Size(), 3U);
    EXPECT_EQ(AsString(in[0]), "ab");
    EXPECT_EQ(in[1].GetSize(), 0U);
    EXPECT_EQ(AsString(in[2]), "cde");
}

// Known defect: the transports do not agree on an empty Parts. zeromq refuses
// it (TransferCode::error). shmem sends a message with zero parts, returns 0,
// and the receiver gets a Receive() with zero parts. Enable this test when
// both transports have the same behavior.
TEST_P(PartsTransfer, DISABLED_EmptyPartsIsRefused)
{
    ProgOptions config;
    test::Configure(config, tools::Uuid(), GetParam().expandedShmMetadata);
    auto factory = TransportFactory::CreateTransportFactory(GetParam().transport, tools::Uuid(), &config);

    Channel push("push", "push", factory);
    Channel pull("pull", "pull", factory);
    test::IpcAddress const address("parts_transfer_empty");
    ASSERT_TRUE(pull.Bind(address));
    ASSERT_TRUE(push.Connect(address));

    Parts empty;
    EXPECT_EQ(push.Send(empty, 1000), static_cast<int64_t>(TransferCode::error));
    Parts in;
    EXPECT_EQ(pull.Receive(in, 100), static_cast<int64_t>(TransferCode::timeout));
}

// Known defect: shmem uses different metadata formats for a single message
// and for Parts. Receive(Parts) of a message sent with Send(MessagePtr) reads
// the size of the message as the number of parts, and reads metadata past the
// received data. zeromq receives it as one part.
TEST_P(PartsTransfer, DISABLED_SingleSendMultipartReceive)
{
    ProgOptions config;
    test::Configure(config, tools::Uuid(), GetParam().expandedShmMetadata);
    auto factory = TransportFactory::CreateTransportFactory(GetParam().transport, tools::Uuid(), &config);

    Channel push("push", "push", factory);
    Channel pull("pull", "pull", factory);
    test::IpcAddress const address("parts_transfer_single");
    ASSERT_TRUE(pull.Bind(address));
    ASSERT_TRUE(push.Connect(address));

    auto out(push.NewSimpleMessage("single"));
    ASSERT_EQ(push.Send(out, 5000), 6);
    Parts in;
    ASSERT_EQ(pull.Receive(in, 5000), 6);
    ASSERT_EQ(in.Size(), 1U);
    EXPECT_EQ(AsString(in[0]), "single");
}

INSTANTIATE_TEST_SUITE_P(, PartsTransfer, test::AllTransports(), test::TransportParamName());

} // namespace
