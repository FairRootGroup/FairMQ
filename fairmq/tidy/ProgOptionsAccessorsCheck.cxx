/********************************************************************************
 *    Copyright (C) 2026 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <fairmq/tidy/ProgOptionsAccessorsCheck.h>

#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>

namespace fair::mq::tidy {

using namespace clang;
using namespace clang::ast_matchers;

void ProgOptionsAccessorsCheck::registerMatchers(MatchFinder* finder)
{
    auto const progOptions = cxxRecordDecl(hasName("::fair::mq::ProgOptions"));
    finder->addMatcher(
        cxxMemberCallExpr(
            callee(cxxMethodDecl(hasAnyName("GetValue", "SetValue", "GetStringValue"),
                                 ofClass(progOptions))
                       .bind("method")),
            unless(hasAncestor(cxxMethodDecl(ofClass(progOptions)))),
            unless(isInTemplateInstantiation()),
            // The result of SetValue() is used if the call is not a statement
            // of its own.
            optionally(hasParent(compoundStmt().bind("statement"))),
            optionally(hasParent(exprWithCleanups(hasParent(compoundStmt().bind("statement"))))))
            .bind("call"),
        this);
}

void ProgOptionsAccessorsCheck::check(MatchFinder::MatchResult const& result)
{
    auto const* call = result.Nodes.getNodeAs<CXXMemberCallExpr>("call");
    auto const* method = result.Nodes.getNodeAs<CXXMethodDecl>("method");
    auto const* callee = dyn_cast<MemberExpr>(call->getCallee()->IgnoreImplicit());
    if (!callee) {
        return;
    }

    auto const name = method->getName();
    StringRef const successor = name == "GetValue"         ? "GetProperty"
                                : name == "GetStringValue" ? "GetPropertyAsString"
                                                           : "SetProperty";
    auto diagnostic =
        diag(callee->getMemberLoc(), "'ProgOptions::%0()' is an old accessor; use '%1()'")
        << name << successor;

    if (callee->getMemberLoc().isMacroID()) {
        return;
    }
    // SetProperty() returns void.
    if (name == "SetValue" && !result.Nodes.getNodeAs<CompoundStmt>("statement")) {
        return;
    }
    diagnostic << FixItHint::CreateReplacement(callee->getMemberLoc(), successor);
    // GetValue() and GetStringValue() return a default-constructed value for an
    // unknown key, GetProperty() and GetPropertyAsString() throw. Pass that
    // default explicitly to keep the behaviour.
    if (name != "SetValue" && call->getRParenLoc().isValid() && !call->getRParenLoc().isMacroID()) {
        diagnostic << FixItHint::CreateInsertion(call->getRParenLoc(), ", {}");
    }
}

}   // namespace fair::mq::tidy
