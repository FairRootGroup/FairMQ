/********************************************************************************
 *    Copyright (C) 2026 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_TIDY_PARTSAPICHECK_H
#define FAIR_MQ_TIDY_PARTSAPICHECK_H

#include <clang-tidy/ClangTidyCheck.h>

namespace fair::mq::tidy {

/// Finds calls of the deprecated `fair::mq::Parts` member functions:
/// - `parts.AtRef(i)` becomes `*parts.At(i)`, or `parts.At(i)->` before a
///   member access,
/// - `parts.AddPart(rawPtr)` becomes `parts.AddPart(fair::mq::MessagePtr(rawPtr))`.
class PartsApiCheck : public clang::tidy::ClangTidyCheck
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

#endif /* FAIR_MQ_TIDY_PARTSAPICHECK_H */
