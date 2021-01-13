/********************************************************************************
 *    Copyright (C) 2019 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_SDK_TRAITS_H
#define FAIR_MQ_SDK_TRAITS_H

#include <asio/associated_allocator.hpp>
#include <asio/associated_executor.hpp>
#include <type_traits>

namespace asio::detail {

/// Specialize to match our coding conventions
template<typename T, typename Executor>
struct associated_executor_impl<T,
                                Executor,
                                std::enable_if_t<is_executor<typename T::ExecutorType>::value>>
{
    using type = typename T::ExecutorType;

    static auto get(const T& obj, const Executor& /*ex = Executor()*/) noexcept -> type
    {
        return obj.GetExecutor();
    }
};

/// Specialize to match our coding conventions
template<typename T, typename Allocator>
struct associated_allocator_impl<T,
                                Allocator,
                                std::enable_if_t<T::AllocatorType>>
{
    using type = typename T::AllocatorType;

    static auto get(const T& obj, const Allocator& /*alloc = Allocator()*/) noexcept -> type
    {
        return obj.GetAllocator();
    }
};

} /* namespace asio::detail */

#endif /* FAIR_MQ_SDK_TRAITS_H */
