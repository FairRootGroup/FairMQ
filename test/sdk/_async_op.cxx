/********************************************************************************
 *    Copyright (C) 2019 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "Fixtures.h"

#include <fairmq/sdk/AsioBase.h>
#include <fairmq/sdk/AsioAsyncOp.h>
#include <asio/steady_timer.hpp>
#include <iostream>
#include <thread>

namespace {

using AsyncOp = fair::mq::test::AsyncOpFixture;

// template <typename Executor, typename Allocator>
// class  : public AsioBase<Executor, Allocator>

TEST_F(AsyncOp, DefaultConstruction)
{
    using namespace fair::mq::sdk;

    AsioAsyncOp<DefaultExecutor, DefaultAllocator, void(std::error_code, int)> op;
    EXPECT_TRUE(op.IsCompleted());
}

TEST_F(AsyncOp, ConstructionWithHandler)
{
    using namespace fair::mq::sdk;

    AsioAsyncOp<DefaultExecutor, DefaultAllocator, void(std::error_code, int)> op(
        [](std::error_code, int) {});
    EXPECT_FALSE(op.IsCompleted());
}

TEST_F(AsyncOp, Complete)
{
    using namespace fair::mq::sdk;

    AsioAsyncOp<DefaultExecutor, DefaultAllocator, void(std::error_code, int)> op(
        [](std::error_code ec, int v) {
            EXPECT_FALSE(ec); // success
            EXPECT_EQ(v, 42);
        });

    EXPECT_FALSE(op.IsCompleted());
    op.Complete(42);
    EXPECT_TRUE(op.IsCompleted());

    EXPECT_THROW(op.Complete(6), RuntimeError); // No double completion!
}

TEST_F(AsyncOp, Cancel)
{
    using namespace fair::mq;

    sdk::AsioAsyncOp<sdk::DefaultExecutor, sdk::DefaultAllocator, void(std::error_code)> op(
        [](std::error_code ec) {
            EXPECT_TRUE(ec); // error
            EXPECT_EQ(ec, MakeErrorCode(ErrorCode::OperationCanceled));
        });

    op.Cancel();
}

TEST_F(AsyncOp, Timeout)
{
    using namespace fair::mq;

    asio::steady_timer timer(mIoContext.get_executor(), std::chrono::milliseconds(50));
    sdk::AsioAsyncOp<sdk::DefaultExecutor, sdk::DefaultAllocator, void(std::error_code)> op(
        mIoContext.get_executor(),
        [&timer](std::error_code ec) {
            timer.cancel();
            std::cout << "Completion with: " << ec.message() << std::endl;
            EXPECT_TRUE(ec); // error
            EXPECT_EQ(ec, MakeErrorCode(ErrorCode::OperationTimeout));
        });
    timer.async_wait([&op](asio::error_code ec) {
        std::cout << "Timer event" << std::endl;
        if (ec != asio::error::operation_aborted) {
            op.Timeout();
        }
    });

    mIoContext.run();
    EXPECT_THROW(op.Complete(), sdk::RuntimeError);
}

TEST_F(AsyncOp, Timeout2)
{
    using namespace fair::mq::sdk;

    asio::steady_timer timer(mIoContext.get_executor(), std::chrono::milliseconds(50));
    AsioAsyncOp<DefaultExecutor, DefaultAllocator, void(std::error_code)> op(
        mIoContext.get_executor(),
        [&timer](std::error_code ec) {
            timer.cancel();
            std::cout << "Completion with: " << ec.message() << std::endl;
            EXPECT_FALSE(ec); // success
        });
    op.Complete(); // Complete before timer
    timer.async_wait([&op](asio::error_code ec) {
        std::cout << "Timer event" << std::endl;
        if (ec != asio::error::operation_aborted) {
            op.Timeout();
        }
    });

    mIoContext.run();
    EXPECT_THROW(op.Complete(), RuntimeError);
}
}   // namespace
