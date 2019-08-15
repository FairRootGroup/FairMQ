/********************************************************************************
 *    Copyright (C) 2019 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_SDK_ASIOASYNCOP_H
#define FAIR_MQ_SDK_ASIOASYNCOP_H

#include <asio/associated_allocator.hpp>
#include <asio/associated_executor.hpp>
#include <asio/executor_work_guard.hpp>
#include <asio/system_executor.hpp>
#include <chrono>
#include <fairmq/sdk/Exceptions.h>
#include <fairmq/sdk/Traits.h>
#include <functional>
#include <memory>
#include <system_error>
#include <type_traits>
#include <utility>

namespace fair {
namespace mq {
namespace sdk {

template<typename... SignatureArgTypes>
struct AsioAsyncOpImplBase
{
    virtual auto Complete(std::error_code, SignatureArgTypes&&...) -> void = 0;
    virtual auto IsCompleted() const -> bool = 0;
};

/**
 * @tparam Executor1 Associated I/O executor, see https://www.boost.org/doc/libs/1_70_0/doc/html/boost_asio/reference/asynchronous_operations.html#boost_asio.reference.asynchronous_operations.associated_i_o_executor
 * @tparam Allocator1 Default allocation strategy, see https://www.boost.org/doc/libs/1_70_0/doc/html/boost_asio/reference/asynchronous_operations.html#boost_asio.reference.asynchronous_operations.allocation_of_intermediate_storage
 */
template<typename Executor1, typename Allocator1, typename Handler, typename... SignatureArgTypes>
struct AsioAsyncOpImpl : AsioAsyncOpImplBase<SignatureArgTypes...>
{
    /// See https://www.boost.org/doc/libs/1_70_0/doc/html/boost_asio/reference/asynchronous_operations.html#boost_asio.reference.asynchronous_operations.allocation_of_intermediate_storage
    using Allocator2 = typename asio::associated_allocator<Handler, Allocator1>::type;

    /// See https://www.boost.org/doc/libs/1_70_0/doc/html/boost_asio/reference/asynchronous_operations.html#boost_asio.reference.asynchronous_operations.associated_completion_handler_executor
    using Executor2 = typename asio::associated_executor<Handler, Executor1>::type;

    /// Ctor
    AsioAsyncOpImpl(const Executor1& ex1, Allocator1&& alloc1, Handler&& handler)
        : fWork1(ex1)
        , fWork2(asio::get_associated_executor(handler, ex1))
        , fHandler(std::move(handler))
        , fAlloc1(std::move(alloc1))
    {}

    auto GetAlloc2() const -> Allocator2 { return asio::get_associated_allocator(fHandler, fAlloc1); }
    auto GetEx2() const -> Executor2 { return asio::get_associated_executor(fWork2); }

    auto Complete(std::error_code ec, SignatureArgTypes&&... args) -> void override
    {
        if (IsCompleted()) {
            throw RuntimeError("Async operation already completed");
        }

        GetEx2().dispatch(
            [=, handler = std::move(fHandler)]() mutable {
                handler(ec, std::forward<SignatureArgTypes>(args)...);
            },
            GetAlloc2());

        fWork1.reset();
        fWork2.reset();
    }

    auto IsCompleted() const -> bool override
    {
        return !fWork1.owns_work() && !fWork2.owns_work();
    }

  private:
    /// See https://www.boost.org/doc/libs/1_70_0/doc/html/boost_asio/reference/asynchronous_operations.html#boost_asio.reference.asynchronous_operations.outstanding_work
    asio::executor_work_guard<Executor1> fWork1;
    asio::executor_work_guard<Executor2> fWork2;
    Handler fHandler;
    Allocator1 fAlloc1;
};

/**
 * @class AsioAsyncOp AsioAsyncOp.h <fairmq/sdk/AsioAsyncOp.h>
 * @tparam Executor Associated I/O executor, see https://www.boost.org/doc/libs/1_70_0/doc/html/boost_asio/reference/asynchronous_operations.html#boost_asio.reference.asynchronous_operations.associated_i_o_executor
 * @tparam Allocator Default allocation strategy, see https://www.boost.org/doc/libs/1_70_0/doc/html/boost_asio/reference/asynchronous_operations.html#boost_asio.reference.asynchronous_operations.allocation_of_intermediate_storage
 * @tparam CompletionSignature
 * @brief Interface for Asio-compliant asynchronous operation, see https://www.boost.org/doc/libs/1_70_0/doc/html/boost_asio/reference/asynchronous_operations.html
 *
 * @par Thread Safety
 * @e Distinct @e objects: Safe.@n
 * @e Shared @e objects: Unsafe.
 *
 * primary template
 */
template<typename Executor, typename Allocator, typename CompletionSignature>
struct AsioAsyncOp
{
};

/**
 * @tparam Executor See primary template
 * @tparam Allocator See primary template
 * @tparam SignatureReturnType Return type of CompletionSignature, see primary template
 * @tparam SignatureFirstArgType Type of first argument of CompletionSignature, see primary template
 * @tparam SignatureArgTypes Types of the rest of arguments of CompletionSignature
 *
 * partial specialization to deconstruct CompletionSignature
 */
template<typename Executor,
         typename Allocator,
         typename SignatureReturnType,
         typename SignatureFirstArgType,
         typename... SignatureArgTypes>
struct AsioAsyncOp<Executor,
                   Allocator,
                   SignatureReturnType(SignatureFirstArgType, SignatureArgTypes...)>
{
    static_assert(std::is_void<SignatureReturnType>::value,
                  "return value of CompletionSignature must be void");
    static_assert(std::is_same<SignatureFirstArgType, std::error_code>::value,
                  "first argument of CompletionSignature must be std::error_code");
    using Duration = std::chrono::milliseconds;

  private:
    using Impl = AsioAsyncOpImplBase<SignatureArgTypes...>;
    using ImplPtr = std::unique_ptr<Impl, std::function<void(Impl*)>>;
    ImplPtr fImpl;

  public:
    /// Default Ctor
    AsioAsyncOp()
        : fImpl(nullptr)
    {}

    /// Ctor with handler
    template<typename Handler>
    AsioAsyncOp(Executor&& ex1, Allocator&& alloc1, Handler&& handler)
        : AsioAsyncOp()
    {
        // Async operation type to be allocated and constructed
        using Op = AsioAsyncOpImpl<Executor, Allocator, Handler, SignatureArgTypes...>;

        // Create allocator for concrete op type
        // Allocator2, see https://www.boost.org/doc/libs/1_70_0/doc/html/boost_asio/reference/asynchronous_operations.html#boost_asio.reference.asynchronous_operations.allocation_of_intermediate_storage
        using OpAllocator =
            typename std::allocator_traits<typename Op::Allocator2>::template rebind_alloc<Op>;
        OpAllocator opAlloc;

        // Allocate memory
        auto mem(std::allocator_traits<OpAllocator>::allocate(opAlloc, 1));

        // Construct object
        auto ptr(new (mem) Op(std::forward<Executor>(ex1),
                              std::forward<Allocator>(alloc1),
                              std::forward<Handler>(handler)));

        // Assign ownership to this object
        fImpl = ImplPtr(ptr, [opAlloc](Impl* p) mutable {
            std::allocator_traits<OpAllocator>::deallocate(opAlloc, static_cast<Op*>(p), 1);
        });
    }

    /// Ctor with handler #2
    template<typename Handler>
    AsioAsyncOp(Executor&& ex1, Handler&& handler)
        : AsioAsyncOp(std::forward<Executor>(ex1), Allocator(), std::forward<Handler>(handler))
    {}

    /// Ctor with handler #3
    template<typename Handler>
    explicit AsioAsyncOp(Handler&& handler)
        : AsioAsyncOp(asio::system_executor(), std::forward<Handler>(handler))
    {}

    auto IsCompleted() -> bool { return (fImpl == nullptr) || fImpl->IsCompleted(); }

    auto Complete(std::error_code ec, SignatureArgTypes&&... args) -> void
    {
        if(IsCompleted()) {
            throw RuntimeError("Async operation already completed");
        }

        fImpl->Complete(ec, std::forward<SignatureArgTypes>(args)...);
        fImpl.reset(nullptr);
    }

    auto Complete(SignatureArgTypes&&... args) -> void
    {
        Complete(std::error_code(), std::forward<SignatureArgTypes>(args)...);
    }

    auto Cancel(SignatureArgTypes&&... args) -> void
    {
        Complete(std::make_error_code(std::errc::operation_canceled),
                 std::forward<SignatureArgTypes>(args)...);
    }
};

} /* namespace sdk */
} /* namespace mq */
} /* namespace fair */

#endif /* FAIR_MQ_SDK_ASIOASYNCOP_H */

