/********************************************************************************
 *    Copyright (C) 2026 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/


#ifndef FAIR_MQ_TIDY_UNCHECKEDTRANSFERRESULTCHECK_H
#define FAIR_MQ_TIDY_UNCHECKEDTRANSFERRESULTCHECK_H

#include <clang-tidy/ClangTidyCheck.h>

namespace fair::mq::tidy {

/// Finds calls of `Receive()` whose result is discarded. On a timeout, an
/// error or an interrupt (e.g. a state change request) the message stays
/// empty, and only the result tells.
///
/// Option `CheckSend` (default `false`): also report `Send()`. Many devices
/// ignore the result of `Send()` on purpose, so this is off by default.
///
/// Cast the call to `void` to discard the result on purpose.
class UncheckedTransferResultCheck : public clang::tidy::ClangTidyCheck
{
  public:
    UncheckedTransferResultCheck(llvm::StringRef name, clang::tidy::ClangTidyContext* context);

    bool isLanguageVersionSupported(clang::LangOptions const& opts) const override
    {
        return opts.CPlusPlus17;
    }
    void storeOptions(clang::tidy::ClangTidyOptions::OptionMap& opts) override;
    void registerMatchers(clang::ast_matchers::MatchFinder* finder) override;
    void check(clang::ast_matchers::MatchFinder::MatchResult const& result) override;

  private:
    bool const fCheckSend;
};

}   // namespace fair::mq::tidy

#endif /* FAIR_MQ_TIDY_UNCHECKEDTRANSFERRESULTCHECK_H */
