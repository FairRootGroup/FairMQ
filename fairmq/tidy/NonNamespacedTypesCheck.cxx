/********************************************************************************
 * Copyright (C) 2021-2026 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <fairmq/tidy/NonNamespacedTypesCheck.h>

#include <clang/AST/QualTypeNames.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>

namespace fair::mq::tidy {

using namespace clang;
using namespace clang::ast_matchers;

void NonNamespacedTypesCheck::registerMatchers(MatchFinder* finder)
{
    // Match the TypedefTypeLoc only. Its ElaboratedTypeLoc wrapper would
    // report the same spelling a second time.
    finder->addMatcher(
        typeLoc(loc(typedefType(hasDeclaration(
                    typedefNameDecl(hasDeclContext(translationUnitDecl()), hasAttr(attr::Deprecated))
                        .bind("alias")))))
            .bind("loc"),
        this);
}

void NonNamespacedTypesCheck::check(MatchFinder::MatchResult const& result)
{
    auto const* alias = result.Nodes.getNodeAs<TypedefNameDecl>("alias");
    auto const* loc = result.Nodes.getNodeAs<TypeLoc>("loc");
    auto const begin = loc->getBeginLoc();
    if (begin.isInvalid() || begin.isMacroID()) {
        return;
    }

    PrintingPolicy policy(getLangOpts());
    policy.SuppressScope = false;
    policy.SuppressUnwrittenScope = true;
    auto const replacement =
        TypeName::getFullyQualifiedName(alias->getUnderlyingType(), *result.Context, policy);
    if (!StringRef(replacement).starts_with("fair::mq::")) {
        return;
    }

    diag(begin, "%0 is deprecated; use '%1'")
        << alias << replacement << FixItHint::CreateReplacement(loc->getSourceRange(), replacement);
}

}   // namespace fair::mq::tidy
