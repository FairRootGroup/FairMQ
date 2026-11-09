/********************************************************************************
 *    Copyright (C) 2026 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/


#include <fairmq/tidy/Transfer.h>
#include <fairmq/tidy/UseAfterSendCheck.h>

#include <clang-tidy/utils/ExprSequence.h>
#include <clang/AST/DeclTemplate.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/Analysis/CFG.h>
#include <llvm/ADT/SmallPtrSet.h>

#include <algorithm>
#include <memory>
#include <vector>

namespace fair::mq::tidy {

using namespace clang;
using namespace clang::ast_matchers;
using clang::tidy::utils::ExprSequence;
using clang::tidy::utils::StmtToBlockMap;

namespace {

/// True if `type` is `std::unique_ptr<fair::mq::Message>`, i.e. `MessagePtr`.
bool isMessagePtr(QualType type)
{
    auto const* spec = dyn_cast_or_null<ClassTemplateSpecializationDecl>(
        type.getNonReferenceType()->getAsCXXRecordDecl());
    if (!spec || spec->getQualifiedNameAsString() != "std::unique_ptr"
        || spec->getTemplateArgs().size() < 1) {
        return false;
    }
    auto const* pointee = spec->getTemplateArgs()[0].getAsType()->getAsCXXRecordDecl();
    return pointee && pointee->getQualifiedNameAsString() == "fair::mq::Message";
}

struct Accesses
{
    std::vector<DeclRefExpr const*> uses;
    std::vector<Stmt const*> reinits;
};

/// Collects the uses of `var` in `body` and the statements that give `var` a
/// new message.
Accesses collect(VarDecl const& var, Stmt const& body, ASTContext& ctx)
{
    auto const ref = declRefExpr(to(varDecl(equalsNode(&var)))).bind("ref");
    auto const reinit = stmt(anyOf(
        // msg = NewMessage(); msg = std::move(other);
        cxxOperatorCallExpr(hasOverloadedOperatorName("="), hasArgument(0, ref)),
        // msg.reset(); msg.swap(other);
        cxxMemberCallExpr(on(ref), callee(cxxMethodDecl(hasAnyName("reset", "swap")))),
        // msg->Rebuild(...);
        cxxMemberCallExpr(on(cxxOperatorCallExpr(hasOverloadedOperatorName("->"), hasArgument(0, ref))),
                          callee(cxxMethodDecl(hasName("Rebuild")))),
        // Receive(msg, ...); fill(msg);
        callExpr(forEachArgumentWithParam(
                     ignoringParenImpCasts(ref),
                     parmVarDecl(hasType(lValueReferenceType(pointee(unless(isConstQualified())))))),
                 unless(callee(functionDecl(hasAnyName("Send", "::std::move", "::std::forward"))))),
        // a new variable in each iteration of a loop
        declStmt(hasSingleDecl(equalsNode(&var)))));

    Accesses result;
    llvm::SmallPtrSet<DeclRefExpr const*, 8> reinitRefs;
    for (auto const& match : ast_matchers::match(findAll(reinit.bind("reinit")), body, ctx)) {
        result.reinits.push_back(match.getNodeAs<Stmt>("reinit"));
        if (auto const* r = match.getNodeAs<DeclRefExpr>("ref")) {
            reinitRefs.insert(r);
        }
    }
    for (auto const& match : ast_matchers::match(findAll(ref), body, ctx)) {
        auto const* r = match.getNodeAs<DeclRefExpr>("ref");
        if (!reinitRefs.count(r)) {
            result.uses.push_back(r);
        }
    }
    return result;
}

}   // namespace

void UseAfterSendCheck::registerMatchers(MatchFinder* finder)
{
    finder->addMatcher(
        cxxMemberCallExpr(
            transferCall("Send"),
            hasArgument(0, ignoringParenImpCasts(declRefExpr(to(varDecl(hasLocalStorage()).bind("var"))).bind("arg"))),
            forCallable(functionDecl().bind("function")),
            unless(isInTemplateInstantiation()))
            .bind("send"),
        this);
}

void UseAfterSendCheck::check(MatchFinder::MatchResult const& result)
{
    auto& ctx = *result.Context;
    auto const* send = result.Nodes.getNodeAs<CXXMemberCallExpr>("send");
    auto const* arg = result.Nodes.getNodeAs<DeclRefExpr>("arg");
    auto const* var = result.Nodes.getNodeAs<VarDecl>("var");
    auto const* body = result.Nodes.getNodeAs<FunctionDecl>("function")->getBody();
    if (!body || !isMessagePtr(var->getType()) || !isDiscarded(*send, ctx)) {
        return;
    }

    CFG::BuildOptions options;
    options.AddImplicitDtors = true;
    options.AddTemporaryDtors = true;
    options.setAllAlwaysAdd();
    std::unique_ptr<CFG> const cfg = CFG::buildCFG(nullptr, const_cast<Stmt*>(body), &ctx, options);
    if (!cfg) {
        return;
    }
    ExprSequence const sequence(cfg.get(), body, &ctx);
    StmtToBlockMap const blocks(cfg.get(), &ctx);
    auto const* start = blocks.blockContainingStmt(send);
    if (!start) {
        return;
    }
    auto const accesses = collect(*var, *body, ctx);
    auto const& sm = *result.SourceManager;

    // Walk the CFG from the send. In the block of the send look only after
    // the send, unless a loop leads back to it.
    llvm::SmallPtrSet<CFGBlock const*, 16> visited;
    std::vector<std::pair<CFGBlock const*, bool>> work{{start, true}};
    while (!work.empty()) {
        auto const [block, afterSend] = work.back();
        work.pop_back();

        std::vector<Stmt const*> reinits;
        for (auto const* r : accesses.reinits) {
            if (blocks.blockContainingStmt(r) == block && (!afterSend || sequence.potentiallyAfter(r, send))) {
                reinits.push_back(r);
            }
        }
        DeclRefExpr const* use = nullptr;
        for (auto const* u : accesses.uses) {
            if (blocks.blockContainingStmt(u) != block || (afterSend && (u == arg || !sequence.potentiallyAfter(u, send)))) {
                continue;
            }
            if (std::any_of(reinits.begin(), reinits.end(), [&](auto const* r) { return sequence.inSequence(r, u); })) {
                continue;
            }
            if (!use || sm.isBeforeInTranslationUnit(u->getBeginLoc(), use->getBeginLoc())) {
                use = u;
            }
        }
        if (use) {
            diag(use->getBeginLoc(), "'%0' is used after it was sent; a sent message is empty (zeromq) "
                                     "or shared with the receiver (shmem)")
                << var->getName();
            diag(send->getExprLoc(), "sent here", DiagnosticIDs::Note);
            return;
        }
        if (!reinits.empty()) {
            continue;
        }
        for (auto const& succ : block->succs()) {
            if (succ && visited.insert(succ).second) {
                work.emplace_back(succ, false);
            }
        }
    }
}

}   // namespace fair::mq::tidy
