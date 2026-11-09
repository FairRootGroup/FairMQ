/********************************************************************************
 *    Copyright (C) 2026 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/


#include <fairmq/tidy/RunLoopIgnoresStateCheck.h>
#include <fairmq/tidy/Transfer.h>

#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>

namespace fair::mq::tidy {

using namespace clang;
using namespace clang::ast_matchers;

namespace {

AST_MATCHER(Expr, isConstantTrue)
{
    if (Node.isValueDependent()) {
        return false;
    }
    bool value = false;
    return Node.EvaluateAsBooleanCondition(value, Finder->getASTContext()) && value;
}

}   // namespace

void RunLoopIgnoresStateCheck::registerMatchers(MatchFinder* finder)
{
    auto const device = cxxRecordDecl(isSameOrDerivedFrom("::fair::mq::Device"));
    auto const run = cxxMethodDecl(hasName("Run"), parameterCountIs(0), isOverride(), ofClass(device));

    auto const watchesState = hasDescendant(cxxMemberCallExpr(callee(cxxMethodDecl(
        hasAnyName("NewStatePending", "WaitFor"), ofClass(hasName("::fair::mq::Device"))))));
    auto const transfers =
        hasDescendant(cxxMemberCallExpr(anyOf(transferCall("Send"), transferCall("Receive"))));
    auto const leaves = hasDescendant(stmt(anyOf(breakStmt(), returnStmt(), cxxThrowExpr(), gotoStmt())));
    auto const endless = anyOf(whileStmt(hasCondition(expr(isConstantTrue()))),
                               doStmt(hasCondition(expr(isConstantTrue()))),
                               forStmt(unless(hasCondition(expr()))),
                               forStmt(hasCondition(expr(isConstantTrue()))));

    finder->addMatcher(stmt(endless,
                            forFunction(run),
                            unless(watchesState),
                            unless(transfers),
                            unless(leaves),
                            unless(isInTemplateInstantiation()))
                           .bind("loop"),
                       this);
}

void RunLoopIgnoresStateCheck::check(MatchFinder::MatchResult const& result)
{
    auto const* loop = result.Nodes.getNodeAs<Stmt>("loop");
    diag(loop->getBeginLoc(),
         "endless loop in 'Run()' does not check for a state change; the device does not stop "
         "on request; use 'while (!NewStatePending())'");
}

}   // namespace fair::mq::tidy
