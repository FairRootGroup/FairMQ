/********************************************************************************
 *    Copyright (C) 2026 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/


#ifndef FAIR_MQ_TIDY_CHANNELLOOKUPINLOOPCHECK_H
#define FAIR_MQ_TIDY_CHANNELLOOKUPINLOOPCHECK_H

#include <clang-tidy/ClangTidyCheck.h>

namespace fair::mq::tidy {

/// Finds calls of `GetChannel()` and `GetChannels()` that run once per
/// iteration of the device loop: in an override of `ConditionalRun()`, or in
/// a loop in an override of `Run()`. Each call looks up the channel name in a
/// map. Look up the channel once, e.g. in `InitTask()`, and keep a reference.
///
/// The check does not report `Send(msg, "name")` and `Receive(msg, "name")`,
/// which also look up the channel, because they are the usual short form.
class ChannelLookupInLoopCheck : public clang::tidy::ClangTidyCheck
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

#endif /* FAIR_MQ_TIDY_CHANNELLOOKUPINLOOPCHECK_H */
