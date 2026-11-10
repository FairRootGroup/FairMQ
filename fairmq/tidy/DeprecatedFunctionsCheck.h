/********************************************************************************
 *    Copyright (C) 2026 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_TIDY_DEPRECATEDFUNCTIONSCHECK_H
#define FAIR_MQ_TIDY_DEPRECATEDFUNCTIONSCHECK_H

#include <clang-tidy/ClangTidyCheck.h>
#include <clang-tidy/utils/IncludeInserter.h>

namespace fair::mq::tidy {

/// Finds calls of deprecated static member functions that have a free
/// function successor and calls the successor instead:
/// - `fair::mq::Device::GetStateName()` -> `fair::mq::GetStateName()`
/// - `fair::mq::Device::GetTransitionName()` -> `fair::mq::GetTransitionName()`
/// - `fair::mq::{zmq,shmem}::Socket::GetConstant()` -> `fair::mq::zmq::getConstant()`
///
/// It also reports calls of the deprecated `fair::mq::Socket::Close()`. This
/// one has no automatic fix: the socket closes in its destructor.
class DeprecatedFunctionsCheck : public clang::tidy::ClangTidyCheck
{
  public:
    DeprecatedFunctionsCheck(llvm::StringRef name, clang::tidy::ClangTidyContext* context);

    bool isLanguageVersionSupported(clang::LangOptions const& opts) const override
    {
        return opts.CPlusPlus17;
    }
    void registerPPCallbacks(clang::SourceManager const& sm,
                             clang::Preprocessor* pp,
                             clang::Preprocessor* moduleExpanderPP) override;
    void registerMatchers(clang::ast_matchers::MatchFinder* finder) override;
    void check(clang::ast_matchers::MatchFinder::MatchResult const& result) override;
    void storeOptions(clang::tidy::ClangTidyOptions::OptionMap& opts) override;

  private:
    clang::tidy::utils::IncludeInserter fInserter;
};

}   // namespace fair::mq::tidy

#endif /* FAIR_MQ_TIDY_DEPRECATEDFUNCTIONSCHECK_H */
