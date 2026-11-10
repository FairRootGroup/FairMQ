/********************************************************************************
 *    Copyright (C) 2026 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/


#include <fairmq/tidy/UncheckedTransferResultCheck.h>
#include <fairmq/tidy/Transfer.h>

#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>

namespace fair::mq::tidy {

using namespace clang;
using namespace clang::ast_matchers;

UncheckedTransferResultCheck::UncheckedTransferResultCheck(llvm::StringRef name,
                                                           clang::tidy::ClangTidyContext* context)
    : ClangTidyCheck(name, context)
    // Not Options.get<bool>(): its out-of-line specialization does not link
    // across compilers (GCC and Clang mangle its enable_if differently).
    , fCheckSend(Options.get("CheckSend", "false").equals_insensitive("true")
                 || Options.get("CheckSend", "false") == "1")
{}

void UncheckedTransferResultCheck::storeOptions(clang::tidy::ClangTidyOptions::OptionMap& opts)
{
    Options.store(opts, "CheckSend", fCheckSend ? "true" : "false");
}

void UncheckedTransferResultCheck::registerMatchers(MatchFinder* finder)
{
    auto const call = fCheckSend ? cxxMemberCallExpr(anyOf(transferCall("Receive"), transferCall("Send")))
                                 : cxxMemberCallExpr(transferCall("Receive"));
    finder->addMatcher(call.bind("call"), this);
}

void UncheckedTransferResultCheck::check(MatchFinder::MatchResult const& result)
{
    auto const* call = result.Nodes.getNodeAs<CXXMemberCallExpr>("call");
    if (!isDiscarded(*call, *result.Context)) {
        return;
    }
    diag(call->getExprLoc(), "result of '%0()' is not checked; it is negative on a timeout, an "
                             "error or an interrupt")
        << call->getMethodDecl()->getName();
}

}   // namespace fair::mq::tidy
