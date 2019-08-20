/********************************************************************************
 *    Copyright (C) 2019 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_SDK_TOP_H
#define FAIR_MQ_SDK_TOP_H

#include <asio/associated_executor.hpp>
#include <asio/async_result.hpp>
#include <asio/detail/non_const_lvalue.hpp>
#include <asio/error.hpp>
#include <asio/executor.hpp>
#include <cstddef>
#include <fairmq/sdk/DDSSession.h>
#include <fairmq/sdk/Traits.h>
#include <iostream>
#include <ncurses.h>
#include <utility>

namespace fair {
namespace mq {
namespace sdk {

/**
 * @brief fairmq-top ncurses application
 * @tparam Executor Associated I/O executor
 *
 * @par Thread Safety
 * @e Distinct @e objects: Safe.@n
 * @e Shared @e objects: Unsafe.
 */
template<typename Executor>
class BasicTop
{
  public:
    /// Member type of associated I/O executor
    using ExecutorType = Executor;
    /// Associated I/O executor
    auto GetExecutor() const noexcept -> ExecutorType { return fExecutor; }

    /// No default construction
    BasicTop() = delete;
    /// No copy construction
    BasicTop(const BasicTop&) = delete;
    /// No copy assignment
    BasicTop& operator=(const BasicTop&) = delete;
    /// Default move construction
    BasicTop(BasicTop&&) noexcept = default;
    /// Default move assignment
    BasicTop& operator=(BasicTop&&) noexcept = default;

    /// Construct Top application
    BasicTop(ExecutorType ex, DDSSession session)
        : fExecutor(std::move(ex))
        , fDDSSession(std::move(session))
    {
        setlocale(LC_ALL, "");
        initscr();
        cbreak();
        noecho();
        nonl();
        intrflush(stdscr, FALSE);
        keypad(stdscr, TRUE);
        printw("Hello World !!!");
        refresh();
    }

    /// Construct Top application
    explicit BasicTop(DDSSession session)
      : BasicTop(Executor(), std::move(session))
    {}

    ~BasicTop() { endwin(); }

    /**
     * @brief Run application
     * @tparam CompletionToken Asio completion token type
     */
    template<typename CompletionToken>
    auto AsyncRun(CompletionToken&& token)
    {
        asio::async_completion<CompletionToken, void(std::error_code)> completion(token);

        auto ex(asio::get_associated_executor(completion.completion_handler, fExecutor));
        auto alloc(asio::get_associated_allocator(completion.completion_handler));

        ex.post([h = std::move(completion.completion_handler)]() mutable { h(std::error_code()); },
                alloc);

        return completion.result.get();

        // return asio::async_initiate<CompletionToken, void(system::error_code)>(
            // [&](auto handler) {
                // auto ex(asio::get_associated_executor(handler, fExecutor));
                // auto alloc(asio::get_associated_allocator(handler));
//
                // ex.post(
                    // [h = std::move(handler)]() {
                        // typename std::decay<decltype(h)>::type h2(std::move(h));
                        // h2(system::errc::make_error_code(system::errc::success));
                    // },
                    // alloc);
            // },
            // token);
    }

  private:
    ExecutorType fExecutor;
    DDSSession fDDSSession;
}; /* class BasicTop */

/**
 * @class Top Top.h <fairmq/sdk/top/Top.h>
 * @brief Usual instantiation of BasicTop template
 */
using Top = BasicTop<asio::executor>;

} /* namespace sdk */
} /* namespace mq */
} /* namespace fair */

#endif /* FAIR_MQ_SDK_TOP_H */
