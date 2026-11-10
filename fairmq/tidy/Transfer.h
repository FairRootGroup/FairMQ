/********************************************************************************
 *    Copyright (C) 2026 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/


#ifndef FAIR_MQ_TIDY_TRANSFER_H
#define FAIR_MQ_TIDY_TRANSFER_H

#include <clang/AST/ASTContext.h>
#include <clang/AST/Expr.h>
#include <clang/ASTMatchers/ASTMatchers.h>

#include <string>

namespace fair::mq::tidy {

/// Matches `fair::mq::Device`, `fair::mq::Channel`, `fair::mq::Socket` and
/// classes derived from them, i.e. the classes with `Send()` and `Receive()`.
inline auto transferClass()
{
    using namespace clang::ast_matchers;
    return cxxRecordDecl(anyOf(isSameOrDerivedFrom("::fair::mq::Device"),
                               isSameOrDerivedFrom("::fair::mq::Channel"),
                               isSameOrDerivedFrom("::fair::mq::Socket")));
}

/// Matches a call of `Send()` or `Receive()` (as given) of a transfer class.
inline auto transferCall(std::string const& name)
{
    using namespace clang::ast_matchers;
    return cxxMemberCallExpr(callee(cxxMethodDecl(hasName(name), ofClass(transferClass()))));
}

/// True if the value of `e` is discarded: `e` is an expression statement or
/// the increment of a for loop. A cast to void does not count, because it
/// discards the value on purpose.
bool isDiscarded(clang::Expr const& e, clang::ASTContext& ctx);

}   // namespace fair::mq::tidy

#endif /* FAIR_MQ_TIDY_TRANSFER_H */
