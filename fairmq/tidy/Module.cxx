/********************************************************************************
 *    Copyright (C) 2026 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <fairmq/tidy/NonNamespacedTypesCheck.h>

#include <clang-tidy/ClangTidyModule.h>
#include <clang-tidy/ClangTidyModuleRegistry.h>

namespace fair::mq::tidy {

/// The `fairmq-*` checks as a clang-tidy module. Every front end links this
/// module: the `fairmq-tidy` executable and, later, the plugin and clangd.
class Module : public clang::tidy::ClangTidyModule
{
  public:
    void addCheckFactories(clang::tidy::ClangTidyCheckFactories& factories) override
    {
        factories.registerCheck<NonNamespacedTypesCheck>("fairmq-modernize-nonnamespaced-types");
    }
};

}   // namespace fair::mq::tidy

static clang::tidy::ClangTidyModuleRegistry::Add<fair::mq::tidy::Module>
    fairmqModule("fairmq-module", "Adds the FairMQ checks.");

// A front end that links this module statically references this symbol, so
// the linker keeps the registration above.
// NOLINTNEXTLINE(misc-use-internal-linkage)
volatile int FairMQTidyModuleAnchorSource = 0;
