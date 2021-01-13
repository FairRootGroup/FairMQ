/********************************************************************************
 *    Copyright (C) 2019 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_SDK_ASIOBASE_H
#define FAIR_MQ_SDK_ASIOBASE_H

#include <asio/any_io_executor.hpp>
#include <fairmq/sdk/Traits.h>
#include <memory>
#include <utility>

namespace fair::mq::sdk
{

using DefaultExecutor = asio::any_io_executor;
using DefaultAllocator = std::allocator<int>;

/**
 * @class AsioBase AsioBase.h <fairmq/sdk/AsioBase.h>
 * @tparam Executor Associated I/O executor
 * @tparam Allocator Associated default allocator
 * @brief Base for creating Asio-enabled I/O objects
 *
 * @par Thread Safety
 * @e Distinct @e objects: Safe.@n
 * @e Shared @e objects: Unsafe.
 */
template<typename Executor, typename Allocator>
class AsioBase
{
  public:
    /// Member type of associated I/O executor
    using ExecutorType = Executor;
    /// Get associated I/O executor
    auto GetExecutor() const noexcept -> ExecutorType { return fExecutor; }

    /// Member type of associated default allocator
    using AllocatorType = Allocator;
    /// Get associated default allocator
    auto GetAllocator() const noexcept -> AllocatorType { return fAllocator; }

    /// NO default ctor
    AsioBase() = delete;

    /// Construct with associated I/O executor
    explicit AsioBase(Executor ex, Allocator alloc)
        : fExecutor(std::move(ex))
        , fAllocator(std::move(alloc))
    {}

    /// NOT copyable
    AsioBase(const AsioBase&) = delete;
    AsioBase& operator=(const AsioBase&) = delete;

    /// movable
    AsioBase(AsioBase&&) noexcept = default;
    AsioBase& operator=(AsioBase&&) noexcept = default;

    ~AsioBase() = default;

  private:
    ExecutorType fExecutor;
    AllocatorType fAllocator;
};

} // namespace fair::mq::sdk

#endif /* FAIR_MQ_SDK_ASIOBASE_H */
