/********************************************************************************
 *    Copyright (C) 2026 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/


#include <fairmq/tidy/ChannelLookupInLoopCheck.h>

#include <clang/AST/ParentMapContext.h>
#include <clang/AST/StmtCXX.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>

namespace fair::mq::tidy {

using namespace clang;
using namespace clang::ast_matchers;

namespace {

/// True if `call` runs once per iteration of a loop: it is in the condition,
/// the increment or the body of a loop, and not in a lambda in between.
bool inLoop(Expr const& call, ASTContext& ctx)
{
    DynTypedNode node = DynTypedNode::create(call);
    while (true) {
        auto const parents = ctx.getParents(node);
        if (parents.size() != 1) {
            return false;
        }
        auto const& parent = parents[0];
        auto const* stmt = node.get<Stmt>();
        if (parent.get<LambdaExpr>() || parent.get<FunctionDecl>()) {
            return false;
        } else if (parent.get<WhileStmt>() || parent.get<DoStmt>()) {
            return true;
        } else if (auto const* s = parent.get<ForStmt>()) {
            if (stmt != s->getInit()) {
                return true;
            }
        } else if (auto const* s = parent.get<CXXForRangeStmt>()) {
            if (stmt == s->getBody()) {
                return true;
            }
        }
        node = parent;
    }
}

}   // namespace

void ChannelLookupInLoopCheck::registerMatchers(MatchFinder* finder)
{
    auto const device = cxxRecordDecl(isSameOrDerivedFrom("::fair::mq::Device"));
    auto const lookup = cxxMemberCallExpr(
        callee(cxxMethodDecl(hasAnyName("GetChannel", "GetChannels"), ofClass(hasName("::fair::mq::Device")))),
        unless(isInTemplateInstantiation()));

    finder->addMatcher(
        cxxMemberCallExpr(lookup,
                          forFunction(cxxMethodDecl(hasName("ConditionalRun"), isOverride(), ofClass(device))))
            .bind("conditionalRun"),
        this);
    finder->addMatcher(
        cxxMemberCallExpr(lookup,
                          forFunction(cxxMethodDecl(hasName("Run"), parameterCountIs(0), isOverride(), ofClass(device))))
            .bind("run"),
        this);
}

void ChannelLookupInLoopCheck::check(MatchFinder::MatchResult const& result)
{
    auto const* call = result.Nodes.getNodeAs<CXXMemberCallExpr>("conditionalRun");
    char const* where = "'ConditionalRun()'";
    if (!call) {
        call = result.Nodes.getNodeAs<CXXMemberCallExpr>("run");
        where = "a loop in 'Run()'";
        if (!inLoop(*call, *result.Context)) {
            return;
        }
    }
    diag(call->getExprLoc(),
         "'%0()' looks up the channel by name in each iteration of %1; look it up once, e.g. in "
         "'InitTask()', and keep a reference")
        << call->getMethodDecl()->getName() << where;
}

}   // namespace fair::mq::tidy
