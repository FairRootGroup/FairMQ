/********************************************************************************
 *    Copyright (C) 2026 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <fairmq/tidy/DeprecatedFunctionsCheck.h>

#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/Lex/Preprocessor.h>

namespace fair::mq::tidy {

using namespace clang;
using namespace clang::ast_matchers;

namespace {

struct Successor
{
    StringRef function;   // fully qualified successor
    StringRef header;     // header that declares it, empty if always available
};

Successor successorOf(CXXMethodDecl const& method)
{
    auto const name = method.getName();
    if (name == "GetStateName") {
        return {"fair::mq::GetStateName", ""};
    }
    if (name == "GetTransitionName") {
        return {"fair::mq::GetTransitionName", ""};
    }
    if (name == "GetConstant") {
        return {"fair::mq::zmq::getConstant", "<fairmq/zeromq/Common.h>"};
    }
    return {};
}

}   // namespace

DeprecatedFunctionsCheck::DeprecatedFunctionsCheck(StringRef name, clang::tidy::ClangTidyContext* context)
    : ClangTidyCheck(name, context)
    , fInserter(Options.getLocalOrGlobal("IncludeStyle", clang::tidy::utils::IncludeSorter::IS_LLVM),
                areDiagsSelfContained())
{}

void DeprecatedFunctionsCheck::registerPPCallbacks(SourceManager const& /*sm*/,
                                                   Preprocessor* pp,
                                                   Preprocessor* /*moduleExpanderPP*/)
{
    fInserter.registerPreprocessor(pp);
}

void DeprecatedFunctionsCheck::storeOptions(clang::tidy::ClangTidyOptions::OptionMap& opts)
{
    Options.store(opts, "IncludeStyle", fInserter.getStyle());
}

void DeprecatedFunctionsCheck::registerMatchers(MatchFinder* finder)
{
    auto const withSuccessor = cxxMethodDecl(
        anyOf(allOf(hasAnyName("GetStateName", "GetTransitionName"),
                    ofClass(hasName("::fair::mq::Device"))),
              allOf(hasName("GetConstant"),
                    ofClass(hasAnyName("::fair::mq::zmq::Socket", "::fair::mq::shmem::Socket")))));
    finder->addMatcher(
        callExpr(callee(withSuccessor.bind("method")),
                 // The successors forward to the same implementation.
                 unless(hasAncestor(cxxMethodDecl(ofClass(
                     hasAnyName("::fair::mq::Device", "::fair::mq::zmq::Socket", "::fair::mq::shmem::Socket"))))),
                 unless(isInTemplateInstantiation()))
            .bind("call"),
        this);

    finder->addMatcher(
        cxxMemberCallExpr(callee(cxxMethodDecl(hasName("Close"),
                                               ofClass(isSameOrDerivedFrom("::fair::mq::Socket")))),
                          unless(hasAncestor(cxxRecordDecl(isSameOrDerivedFrom("::fair::mq::Socket")))),
                          unless(isInTemplateInstantiation()))
            .bind("close"),
        this);
}

void DeprecatedFunctionsCheck::check(MatchFinder::MatchResult const& result)
{
    if (auto const* close = result.Nodes.getNodeAs<CXXMemberCallExpr>("close")) {
        diag(close->getExprLoc(),
             "'Socket::Close()' is deprecated; the socket closes when it is destroyed");
        return;
    }

    auto const* call = result.Nodes.getNodeAs<CallExpr>("call");
    auto const* method = result.Nodes.getNodeAs<CXXMethodDecl>("method");
    auto const successor = successorOf(*method);
    if (successor.function.empty()) {
        return;
    }

    // The callee spelling: `GetStateName`, `Device::GetStateName`,
    // `device.GetStateName` or `device->GetStateName`.
    Expr const* callee = call->getCallee()->IgnoreImplicit();
    auto diagnostic = diag(call->getBeginLoc(), "'%0' is deprecated; use '%1'")
                      << method->getQualifiedNameAsString() << successor.function;

    if (callee->getBeginLoc().isMacroID() || callee->getEndLoc().isMacroID()) {
        return;
    }
    // Do not drop an object expression that has side effects.
    if (auto const* member = dyn_cast<MemberExpr>(callee);
        member && member->getBase()->HasSideEffects(*result.Context)) {
        return;
    }
    diagnostic << FixItHint::CreateReplacement(callee->getSourceRange(), successor.function);
    if (!successor.header.empty()) {
        diagnostic << fInserter.createIncludeInsertion(
            result.SourceManager->getFileID(call->getBeginLoc()), successor.header);
    }
}

}   // namespace fair::mq::tidy
