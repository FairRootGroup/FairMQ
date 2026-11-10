/********************************************************************************
 * Copyright (C) 2021-2026 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_TIDY_NONNAMESPACEDTYPESCHECK_H
#define FAIR_MQ_TIDY_NONNAMESPACEDTYPESCHECK_H

#include <clang-tidy/ClangTidyCheck.h>

namespace fair::mq::tidy {

/// Finds uses of the deprecated global type aliases (e.g. `FairMQDevice`) and
/// replaces them with the namespaced type they alias (e.g. `fair::mq::Device`).
///
/// A global alias qualifies if it is marked `[[deprecated]]` and its target
/// type lives in namespace `fair::mq`.
class NonNamespacedTypesCheck : public clang::tidy::ClangTidyCheck
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

#endif /* FAIR_MQ_TIDY_NONNAMESPACEDTYPESCHECK_H */
