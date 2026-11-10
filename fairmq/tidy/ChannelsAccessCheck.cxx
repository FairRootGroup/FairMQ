/********************************************************************************
 *    Copyright (C) 2026 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <fairmq/tidy/ChannelsAccessCheck.h>

#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/Lex/Lexer.h>

#include <string>

namespace fair::mq::tidy {

using namespace clang;
using namespace clang::ast_matchers;

void ChannelsAccessCheck::registerMatchers(MatchFinder* finder)
{
    finder->addMatcher(
        memberExpr(member(fieldDecl(hasName("fChannels"),
                                    hasDeclContext(cxxRecordDecl(hasName("::fair::mq::Device"))))),
                   // The Device implementation itself may use its own member.
                   unless(hasAncestor(cxxMethodDecl(ofClass(hasName("::fair::mq::Device"))))),
                   unless(isInTemplateInstantiation()))
            .bind("channels"),
        this);
}

namespace {

/// If `object` is the object of a call `object.at(arg)`, return that call.
CXXMemberCallExpr const* atCallOn(Expr const& object, ASTContext& ctx)
{
    auto const parents = ctx.getParents(object);
    auto const* member = parents.size() == 1 ? parents[0].get<MemberExpr>() : nullptr;
    if (!member) {
        return nullptr;
    }
    auto const grandParents = ctx.getParents(*member);
    auto const* call = grandParents.size() == 1 ? grandParents[0].get<CXXMemberCallExpr>() : nullptr;
    if (!call || call->getNumArgs() != 1 || !call->getMethodDecl()
        || call->getMethodDecl()->getName() != "at") {
        return nullptr;
    }
    return call;
}

}   // namespace

void ChannelsAccessCheck::check(MatchFinder::MatchResult const& result)
{
    auto const* channels = result.Nodes.getNodeAs<MemberExpr>("channels");
    if (channels->getBeginLoc().isMacroID() || channels->getEndLoc().isMacroID()) {
        return;
    }
    auto const text = [&](Expr const* e) {
        return Lexer::getSourceText(CharSourceRange::getTokenRange(e->getSourceRange()),
                                    *result.SourceManager,
                                    getLangOpts())
            .str();
    };

    // fChannels.at(name).at(index) -> GetChannel(name, index)
    auto const* byName = atCallOn(*channels, *result.Context);
    auto const* byIndex = byName ? atCallOn(*byName, *result.Context) : nullptr;
    if (byIndex && !byIndex->getEndLoc().isMacroID()) {
        std::string prefix;
        if (!channels->isImplicitAccess()) {
            prefix = text(channels->getBase()) + (channels->isArrow() ? "->" : ".");
        }
        auto const replacement =
            prefix + "GetChannel(" + text(byName->getArg(0)) + ", " + text(byIndex->getArg(0)) + ")";
        diag(channels->getMemberLoc(), "'fChannels' is deprecated; use '%0'")
            << replacement << FixItHint::CreateReplacement(byIndex->getSourceRange(), replacement);
        return;
    }

    diag(channels->getMemberLoc(), "'fChannels' is deprecated; use 'GetChannels()'")
        << FixItHint::CreateReplacement(channels->getMemberLoc(), "GetChannels()");
}

}   // namespace fair::mq::tidy
