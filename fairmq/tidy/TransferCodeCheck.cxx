/********************************************************************************
 *    Copyright (C) 2026 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/


#include <fairmq/tidy/TransferCodeCheck.h>
#include <fairmq/tidy/Transfer.h>

#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>

namespace fair::mq::tidy {

using namespace clang;
using namespace clang::ast_matchers;

void TransferCodeCheck::registerMatchers(MatchFinder* finder)
{
    auto const transfer = expr(ignoringParenImpCasts(
        cxxMemberCallExpr(anyOf(transferCall("Send"), transferCall("Receive")))));
    auto const negativeLiteral = expr(ignoringParenImpCasts(
        unaryOperator(hasOperatorName("-"), hasUnaryOperand(integerLiteral().bind("literal")))
            .bind("code")));

    finder->addMatcher(binaryOperator(hasAnyOperatorName("==", "!="),
                                      hasOperands(transfer, negativeLiteral),
                                      unless(isInTemplateInstantiation())),
                       this);
}

void TransferCodeCheck::check(MatchFinder::MatchResult const& result)
{
    auto const* code = result.Nodes.getNodeAs<UnaryOperator>("code");
    auto const* literal = result.Nodes.getNodeAs<IntegerLiteral>("literal");

    char const* name = nullptr;
    switch (literal->getValue().getLimitedValue()) {
        case 1: name = "error"; break;
        case 2: name = "timeout"; break;
        case 3: name = "interrupted"; break;
        default: return;
    }

    auto diagnostic = diag(code->getBeginLoc(), "magic number -%0 for 'fair::mq::TransferCode::%1'")
                      << static_cast<unsigned>(literal->getValue().getLimitedValue()) << name;
    if (code->getBeginLoc().isMacroID() || code->getEndLoc().isMacroID()) {
        return;
    }
    diagnostic << FixItHint::CreateReplacement(
        code->getSourceRange(),
        (llvm::Twine("static_cast<int64_t>(fair::mq::TransferCode::") + name + ")").str());
}

}   // namespace fair::mq::tidy
