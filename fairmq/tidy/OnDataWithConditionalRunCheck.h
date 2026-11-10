/********************************************************************************
 *    Copyright (C) 2026 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_TIDY_ONDATAWITHCONDITIONALRUNCHECK_H
#define FAIR_MQ_TIDY_ONDATAWITHCONDITIONALRUNCHECK_H

#include <clang-tidy/ClangTidyCheck.h>
#include <llvm/ADT/MapVector.h>

namespace fair::mq::tidy {

/// Finds devices that register a data callback with `OnData()` and also
/// override `ConditionalRun()`. A device with data callbacks runs only the
/// callbacks, so `ConditionalRun()` never runs.
class OnDataWithConditionalRunCheck : public clang::tidy::ClangTidyCheck
{
  public:
    using ClangTidyCheck::ClangTidyCheck;

    bool isLanguageVersionSupported(clang::LangOptions const& opts) const override
    {
        return opts.CPlusPlus17;
    }
    void registerMatchers(clang::ast_matchers::MatchFinder* finder) override;
    void check(clang::ast_matchers::MatchFinder::MatchResult const& result) override;
    void onEndOfTranslationUnit() override;

  private:
    llvm::MapVector<clang::CXXRecordDecl const*, clang::CXXMethodDecl const*> fConditionalRuns;
    llvm::MapVector<clang::CXXRecordDecl const*, clang::Expr const*> fOnDataCalls;
};

}   // namespace fair::mq::tidy

#endif /* FAIR_MQ_TIDY_ONDATAWITHCONDITIONALRUNCHECK_H */
