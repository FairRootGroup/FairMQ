/********************************************************************************
 *    Copyright (C) 2026 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <fairmq/tidy/PartsApiCheck.h>

#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/Lex/Lexer.h>

namespace fair::mq::tidy {

using namespace clang;
using namespace clang::ast_matchers;

void PartsApiCheck::registerMatchers(MatchFinder* finder)
{
    auto const parts = cxxRecordDecl(hasName("::fair::mq::Parts"));
    auto const notInParts = unless(hasAncestor(cxxMethodDecl(ofClass(parts))));

    finder->addMatcher(
        cxxMemberCallExpr(callee(cxxMethodDecl(hasName("AtRef"), ofClass(parts))),
                          notInParts,
                          unless(isInTemplateInstantiation()))
            .bind("atRef"),
        this);

    finder->addMatcher(
        cxxMemberCallExpr(
            callee(cxxMethodDecl(hasName("AddPart"),
                                 ofClass(parts),
                                 parameterCountIs(1),
                                 hasParameter(0, parmVarDecl(hasType(pointerType()))))),
            hasArgument(0, expr().bind("rawPtr")),
            notInParts,
            unless(isInTemplateInstantiation()))
            .bind("addPart"),
        this);
}

void PartsApiCheck::check(MatchFinder::MatchResult const& result)
{
    auto& sm = *result.SourceManager;

    if (auto const* call = result.Nodes.getNodeAs<CXXMemberCallExpr>("atRef")) {
        auto const* callee = dyn_cast<MemberExpr>(call->getCallee()->IgnoreImplicit());
        auto diagnostic = diag(call->getExprLoc(), "'Parts::AtRef()' is deprecated; use 'Parts::At()'");
        if (!callee || call->getBeginLoc().isMacroID() || call->getEndLoc().isMacroID()) {
            return;
        }
        diagnostic << FixItHint::CreateReplacement(callee->getMemberLoc(), "At");

        // parts.AtRef(i).GetSize() -> parts.At(i)->GetSize()
        // The member access can see the call through implicit nodes, e.g. a
        // NoOp cast to const for a const method.
        DynTypedNode node = DynTypedNode::create(*call);
        MemberExpr const* outer = nullptr;
        while (true) {
            auto const parents = result.Context->getParents(node);
            if (parents.size() != 1) {
                break;
            }
            node = parents[0];
            if (node.get<ImplicitCastExpr>() || node.get<MaterializeTemporaryExpr>()) {
                continue;
            }
            outer = node.get<MemberExpr>();
            break;
        }
        if (outer && !outer->isArrow() && outer->getOperatorLoc().isValid()
            && !outer->getOperatorLoc().isMacroID()) {
            diagnostic << FixItHint::CreateReplacement(outer->getOperatorLoc(), "->");
        } else {
            // A postfix call binds tighter than a prefix '*'.
            diagnostic << FixItHint::CreateInsertion(call->getBeginLoc(), "*");
        }
        return;
    }

    if (auto const* call = result.Nodes.getNodeAs<CXXMemberCallExpr>("addPart")) {
        auto const* arg = result.Nodes.getNodeAs<Expr>("rawPtr");
        auto diagnostic = diag(call->getExprLoc(),
                               "'Parts::AddPart(Message*)' is deprecated; pass a 'fair::mq::MessagePtr'");
        if (arg->getBeginLoc().isMacroID() || arg->getEndLoc().isMacroID()) {
            return;
        }
        auto const text = Lexer::getSourceText(
            CharSourceRange::getTokenRange(arg->getSourceRange()), sm, getLangOpts());
        diagnostic << FixItHint::CreateReplacement(
            arg->getSourceRange(), ("fair::mq::MessagePtr(" + text + ")").str());
    }
}

}   // namespace fair::mq::tidy
