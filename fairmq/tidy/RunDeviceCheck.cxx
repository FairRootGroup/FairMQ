/********************************************************************************
 *    Copyright (C) 2026 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <fairmq/tidy/RunDeviceCheck.h>

#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Lex/Lexer.h>
#include <clang/Lex/PPCallbacks.h>
#include <clang/Lex/Preprocessor.h>
#include <llvm/Config/llvm-config.h>

#include <memory>
#include <string>
#include <vector>

namespace fair::mq::tidy {

using namespace clang;
using namespace clang::ast_matchers;

namespace {

class Callbacks : public PPCallbacks
{
  public:
    Callbacks(std::optional<RunDeviceCheck::Include>& include, SourceManager const& sm)
        : fInclude(include)
        , fSM(sm)
    {}

    void InclusionDirective(SourceLocation /*hashLoc*/,
                            Token const& /*includeTok*/,
                            StringRef fileName,
                            bool isAngled,
                            CharSourceRange fileNameRange,
                            OptionalFileEntryRef /*file*/,
                            StringRef /*searchPath*/,
                            StringRef /*relativePath*/,
#if LLVM_VERSION_MAJOR >= 19
                            Module const* /*suggestedModule*/,
                            bool /*moduleImported*/,
#else
                            Module const* /*imported*/,
#endif
                            SrcMgr::CharacteristicKind /*fileType*/) override
    {
        if (fSM.isWrittenInMainFile(fileNameRange.getBegin())
            && (fileName == "runFairMQDevice.h" || fileName.ends_with("/runFairMQDevice.h"))) {
            fInclude = RunDeviceCheck::Include{fileNameRange, isAngled};
        }
    }

  private:
    std::optional<RunDeviceCheck::Include>& fInclude;
    SourceManager const& fSM;
};

/// Collects the return statements of a function body, but not those of nested
/// lambdas or local classes.
class ReturnCollector : public RecursiveASTVisitor<ReturnCollector>
{
  public:
    bool VisitReturnStmt(ReturnStmt* s)
    {
        fReturns.push_back(s);
        return true;
    }
    bool TraverseLambdaExpr(LambdaExpr*) { return true; }
    bool TraverseCXXRecordDecl(CXXRecordDecl*) { return true; }

    std::vector<ReturnStmt const*> fReturns;
};

}   // namespace

void RunDeviceCheck::registerPPCallbacks(SourceManager const& sm,
                                         Preprocessor* pp,
                                         Preprocessor* /*moduleExpanderPP*/)
{
    fInclude.reset();
    pp->addPPCallbacks(std::make_unique<Callbacks>(fInclude, sm));
}

void RunDeviceCheck::registerMatchers(MatchFinder* finder)
{
    auto const progOptions = cxxRecordDecl(hasName("::fair::mq::ProgOptions"));
    finder->addMatcher(
        functionDecl(hasName("::getDevice"),
                     isDefinition(),
                     isExpansionInMainFile(),
                     returns(hasCanonicalType(pointerType(
                         pointee(hasDeclaration(cxxRecordDecl(hasName("::fair::mq::Device"))))))),
                     parameterCountIs(1),
                     hasParameter(0, parmVarDecl(hasType(lValueReferenceType(pointee(qualType(
                                                     isConstQualified(),
                                                     hasCanonicalType(hasDeclaration(progOptions)))))))
                                         .bind("param")))
            .bind("getDevice"),
        this);
}

void RunDeviceCheck::check(MatchFinder::MatchResult const& result)
{
    auto const* fn = result.Nodes.getNodeAs<FunctionDecl>("getDevice");
    auto const* param = result.Nodes.getNodeAs<ParmVarDecl>("param");
    auto& sm = *result.SourceManager;
    auto const& lo = getLangOpts();

    auto diagnostic = diag(fn->getLocation(),
                           "'runFairMQDevice.h' and a raw 'fair::mq::Device*' from 'getDevice()' are "
                           "superseded; use 'fairmq/runDevice.h' and return a "
                           "'std::unique_ptr<fair::mq::Device>'");
    if (!fInclude) {
        return;   // the include is in another file: no consistent fix
    }

    std::vector<FixItHint> fixes;
    auto const text = [&](CharSourceRange range) { return Lexer::getSourceText(range, sm, lo).str(); };
    auto const isMacro = [](SourceRange r) { return r.getBegin().isMacroID() || r.getEnd().isMacroID(); };

    // #include <runFairMQDevice.h> -> #include <fairmq/runDevice.h>
    fixes.push_back(FixItHint::CreateReplacement(
        fInclude->fileNameRange, fInclude->isAngled ? "<fairmq/runDevice.h>" : "\"fairmq/runDevice.h\""));

    // FairMQDevicePtr -> std::unique_ptr<fair::mq::Device>
    auto const returnRange = fn->getReturnTypeSourceRange();
    if (returnRange.isInvalid() || isMacro(returnRange)) {
        return;
    }
    fixes.push_back(FixItHint::CreateReplacement(returnRange, "std::unique_ptr<fair::mq::Device>"));

    // const fair::mq::ProgOptions& cfg -> fair::mq::ProgOptions& cfg
    if (isMacro(param->getSourceRange())) {
        return;
    }
    if (param->getName().empty()) {
        fixes.push_back(FixItHint::CreateReplacement(param->getSourceRange(), "fair::mq::ProgOptions&"));
    } else {
        fixes.push_back(FixItHint::CreateReplacement(
            CharSourceRange::getCharRange(param->getBeginLoc(), param->getLocation()),
            "fair::mq::ProgOptions& "));
    }

    // return new T(args) -> return std::make_unique<T>(args)
    ReturnCollector collector;
    collector.TraverseStmt(fn->getBody());
    for (auto const* ret : collector.fReturns) {
        auto const* value = ret->getRetValue() ? ret->getRetValue()->IgnoreImpCasts() : nullptr;
        if (!value || isa<CXXNullPtrLiteralExpr>(value)) {
            continue;   // nullptr converts to std::unique_ptr
        }
        auto const* newExpr = dyn_cast<CXXNewExpr>(value);
        if (!newExpr || newExpr->isArray() || newExpr->getNumPlacementArgs() > 0
            || isMacro(newExpr->getSourceRange())) {
            return;
        }
        auto const typeRange = newExpr->getAllocatedTypeSourceInfo()->getTypeLoc().getSourceRange();
        auto const afterType = Lexer::getLocForEndOfToken(typeRange.getEnd(), 0, sm, lo);
        auto const afterNew = Lexer::getLocForEndOfToken(newExpr->getEndLoc(), 0, sm, lo);
        std::string init;   // empty for `new T`
        if (afterType != afterNew) {
            init = text(CharSourceRange::getCharRange(afterType, afterNew));
        }
        StringRef trimmed = StringRef(init).ltrim();
        if (trimmed.empty()) {
            trimmed = "()";
        } else if (!trimmed.starts_with("(")) {
            return;   // new T{...}: braces mean something else in make_unique
        }
        fixes.push_back(FixItHint::CreateReplacement(
            newExpr->getSourceRange(),
            "std::make_unique<" + text(CharSourceRange::getTokenRange(typeRange)) + ">" + trimmed.str()));
    }

    for (auto const& fix : fixes) {
        diagnostic << fix;
    }
}

}   // namespace fair::mq::tidy
