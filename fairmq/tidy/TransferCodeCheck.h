/********************************************************************************
 *    Copyright (C) 2026 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/


#ifndef FAIR_MQ_TIDY_TRANSFERCODECHECK_H
#define FAIR_MQ_TIDY_TRANSFERCODECHECK_H

#include <clang-tidy/ClangTidyCheck.h>

namespace fair::mq::tidy {

/// Finds comparisons of the result of `Send()` or `Receive()` with the
/// magic numbers -1, -2 and -3 and uses `fair::mq::TransferCode` instead:
///
///     Receive(msg, "data") == -2
///  -> Receive(msg, "data") == static_cast<int64_t>(fair::mq::TransferCode::timeout)
class TransferCodeCheck : public clang::tidy::ClangTidyCheck
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

#endif /* FAIR_MQ_TIDY_TRANSFERCODECHECK_H */
