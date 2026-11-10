/********************************************************************************
 *    Copyright (C) 2026 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/


#include <fairmq/tidy/Transfer.h>

#include <clang/AST/ParentMapContext.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/StmtCXX.h>

namespace fair::mq::tidy {

using namespace clang;

bool isDiscarded(Expr const& e, ASTContext& ctx)
{
    DynTypedNode node = DynTypedNode::create(e);
    while (true) {
        auto const parents = ctx.getParents(node);
        if (parents.size() != 1) {
            return false;
        }
        auto const& parent = parents[0];
        if (parent.get<ExprWithCleanups>() || parent.get<ParenExpr>()) {
            node = parent;
            continue;
        }
        auto const* stmt = node.get<Stmt>();
        if (parent.get<CompoundStmt>() || parent.get<CaseStmt>() || parent.get<DefaultStmt>()
            || parent.get<LabelStmt>() || parent.get<AttributedStmt>()) {
            return true;
        }
        if (auto const* s = parent.get<IfStmt>()) {
            return stmt == s->getThen() || stmt == s->getElse();
        }
        if (auto const* s = parent.get<WhileStmt>()) {
            return stmt == s->getBody();
        }
        if (auto const* s = parent.get<DoStmt>()) {
            return stmt == s->getBody();
        }
        if (auto const* s = parent.get<ForStmt>()) {
            return stmt == s->getBody() || stmt == s->getInc();
        }
        if (auto const* s = parent.get<CXXForRangeStmt>()) {
            return stmt == s->getBody();
        }
        return false;
    }
}

}   // namespace fair::mq::tidy
