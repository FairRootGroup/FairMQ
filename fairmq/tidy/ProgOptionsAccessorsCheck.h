/********************************************************************************
 *    Copyright (C) 2026 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_TIDY_PROGOPTIONSACCESSORSCHECK_H
#define FAIR_MQ_TIDY_PROGOPTIONSACCESSORSCHECK_H

#include <clang-tidy/ClangTidyCheck.h>

namespace fair::mq::tidy {

/// Finds calls of the old `fair::mq::ProgOptions` accessors and calls the
/// property API instead:
/// - `GetValue<T>(key)` -> `GetProperty<T>(key, {})`
/// - `GetStringValue(key)` -> `GetPropertyAsString(key, {})`
/// - `SetValue(key, value)` -> `SetProperty(key, value)` (only if the
///   always-zero result of `SetValue` is not used)
///
/// The `{}` default keeps the behaviour for an unknown key: the old accessors
/// return a default-constructed value, the property API throws without it.
class ProgOptionsAccessorsCheck : public clang::tidy::ClangTidyCheck
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

#endif /* FAIR_MQ_TIDY_PROGOPTIONSACCESSORSCHECK_H */
