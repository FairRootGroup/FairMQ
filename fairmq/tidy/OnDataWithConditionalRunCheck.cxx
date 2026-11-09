/********************************************************************************
 *    Copyright (C) 2026 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <fairmq/tidy/OnDataWithConditionalRunCheck.h>

#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>

namespace fair::mq::tidy {

using namespace clang;
using namespace clang::ast_matchers;

void OnDataWithConditionalRunCheck::registerMatchers(MatchFinder* finder)
{
    auto const device = cxxRecordDecl(isDerivedFrom("::fair::mq::Device"));

    finder->addMatcher(
        cxxMethodDecl(hasName("ConditionalRun"), isOverride(), ofClass(device.bind("class")))
            .bind("conditionalRun"),
        this);

    // OnData() on the device itself, in any member function of the device.
    finder->addMatcher(
        cxxMemberCallExpr(
            callee(cxxMethodDecl(hasName("OnData"), ofClass(hasName("::fair::mq::Device")))),
            on(ignoringImpCasts(cxxThisExpr())),
            hasAncestor(cxxMethodDecl(ofClass(device.bind("class")))),
            unless(isInTemplateInstantiation()))
            .bind("onData"),
        this);
}

void OnDataWithConditionalRunCheck::check(MatchFinder::MatchResult const& result)
{
    auto const* cls = result.Nodes.getNodeAs<CXXRecordDecl>("class")->getCanonicalDecl();
    if (auto const* run = result.Nodes.getNodeAs<CXXMethodDecl>("conditionalRun")) {
        // prefer the definition, if any
        if (!fConditionalRuns.count(cls) || run->isThisDeclarationADefinition()) {
            fConditionalRuns[cls] = run;
        }
    } else if (auto const* call = result.Nodes.getNodeAs<Expr>("onData")) {
        fOnDataCalls.insert({cls, call});
    }
}

void OnDataWithConditionalRunCheck::onEndOfTranslationUnit()
{
    for (auto const& [cls, run] : fConditionalRuns) {
        auto const it = fOnDataCalls.find(cls);
        if (it == fOnDataCalls.end()) {
            continue;
        }
        diag(run->getLocation(),
             "%0 overrides 'ConditionalRun()' and registers a data callback with 'OnData()'; "
             "'ConditionalRun()' never runs")
            << cls;
        diag(it->second->getExprLoc(), "data callback registered here", DiagnosticIDs::Note);
    }
    fConditionalRuns.clear();
    fOnDataCalls.clear();
}

}   // namespace fair::mq::tidy
