/********************************************************************************
 *    Copyright (C) 2021 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_TIDY_MODERNIZENONNAMESPACEDTYPES
#define FAIR_MQ_TIDY_MODERNIZENONNAMESPACEDTYPES

#include <clang/AST/AST.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/Basic/Diagnostic.h>
#include <llvm/Support/Casting.h>

namespace fair::mq::tidy {

struct ModernizeNonNamespacedTypes
{
    ModernizeNonNamespacedTypes() = delete;
    ModernizeNonNamespacedTypes(clang::ast_matchers::MatchFinder& finder)
    {
        using namespace clang::ast_matchers;

        // https://clang.llvm.org/docs/LibASTMatchersReference.html
        finder.addMatcher(
            typeLoc(loc(qualType(hasDeclaration(namedDecl(matchesName("::FairMQ")).bind("decl")))))
                .bind("loc"),
            &fCallback);
    }

    struct Callback : clang::ast_matchers::MatchFinder::MatchCallback
    {
        auto run(clang::ast_matchers::MatchFinder::MatchResult const& m) -> void final
        {
            using namespace clang;

            auto const type_loc(m.Nodes.getNodeAs<TypeLoc>("loc"));
            auto const named_decl(m.Nodes.getNodeAs<NamedDecl>("decl"));

            if (auto const type_alias_decl = m.Nodes.getNodeAs<TypeAliasDecl>("decl")) {
                auto const underlying_type(type_alias_decl->getUnderlyingType());

                if (underlying_type.getAsString().rfind("fair::mq::", 0) == 0) {
                    auto& diag_engine(m.Context->getDiagnostics());
                    auto const location(type_loc->getBeginLoc());
                    if (location.isValid()) {
                        auto builder(diag_engine.Report(
                            location,
                            diag_engine.getCustomDiagID(
                                DiagnosticsEngine::Warning,
                                "Modernize non-namespaced type %0 with %1. [%2]")));
                        builder << named_decl;
                        builder << underlying_type;
                        builder << "fairmq-modernize-nonnamespaced-types";

                        builder.AddFixItHint(FixItHint::CreateReplacement(
                            type_loc->getSourceRange(), underlying_type.getAsString()));
                    }
                }
            }
        }
    };

  private:
    Callback fCallback;
};

}   // namespace fair::mq::tidy

#endif /* FAIR_MQ_TIDY_MODERNIZENONNAMESPACEDTYPES */
