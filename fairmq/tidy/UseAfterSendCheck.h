/********************************************************************************
 *    Copyright (C) 2026 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/


#ifndef FAIR_MQ_TIDY_USEAFTERSENDCHECK_H
#define FAIR_MQ_TIDY_USEAFTERSENDCHECK_H

#include <clang-tidy/ClangTidyCheck.h>

namespace fair::mq::tidy {

/// Finds uses of a `fair::mq::MessagePtr` after it was sent. After a
/// successful `Send()` the message is empty with the zeromq transport, and it
/// still points to the sent buffer with the shmem transport, i.e. a write
/// races with the receiver. The same code acts differently per transport.
///
/// The check looks only at sends whose result is discarded. A send whose
/// result is checked can fail, and the code can then use the message again.
/// Assigning a new message, `reset()`, `Rebuild()` and passing the message
/// by non-const reference (e.g. to `Receive()`) start over.
class UseAfterSendCheck : public clang::tidy::ClangTidyCheck
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

#endif /* FAIR_MQ_TIDY_USEAFTERSENDCHECK_H */
