/********************************************************************************
 *    Copyright (C) 2026 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/


#ifndef FAIR_MQ_TIDY_RUNLOOPIGNORESSTATECHECK_H
#define FAIR_MQ_TIDY_RUNLOOPIGNORESSTATECHECK_H

#include <clang-tidy/ClangTidyCheck.h>

namespace fair::mq::tidy {

/// Finds endless loops (`while (true)`, `for (;;)`, ...) in an override of
/// `fair::mq::Device::Run()` that do not look at the state machine. Such a
/// device does not stop when a state change is requested.
///
/// A loop is fine if it calls `NewStatePending()` or `WaitFor()`, calls
/// `Send()` or `Receive()` (an interrupt stops a transfer on a state
/// change), or has a `break`, `return`, `throw` or `goto`.
class RunLoopIgnoresStateCheck : public clang::tidy::ClangTidyCheck
{
  public:
    using ClangTidyCheck::ClangTidyCheck;

    bool isLanguageVersionSupported(clang::LangOptions const& opts) const override
    {
        return opts.CPlusPlus17;
    }
    void registerMatchers(clang::ast_matchers::MatchFinder* finder) override;
    void check(clang::ast_matchers::MatchFinder::MatchResult const& result) override;
};

}   // namespace fair::mq::tidy

#endif /* FAIR_MQ_TIDY_RUNLOOPIGNORESSTATECHECK_H */
