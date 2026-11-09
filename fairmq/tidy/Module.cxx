/********************************************************************************
 *    Copyright (C) 2026 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <fairmq/tidy/ChannelsAccessCheck.h>
#include <fairmq/tidy/DeprecatedFunctionsCheck.h>
#include <fairmq/tidy/DeprecatedHeadersCheck.h>
#include <fairmq/tidy/NonNamespacedTypesCheck.h>
#include <fairmq/tidy/PartsApiCheck.h>
#include <fairmq/tidy/ProgOptionsAccessorsCheck.h>
#include <fairmq/tidy/RunDeviceCheck.h>
#include <fairmq/tidy/TransferCodeCheck.h>
#include <fairmq/tidy/UncheckedTransferResultCheck.h>
#include <fairmq/tidy/UseAfterSendCheck.h>

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
        factories.registerCheck<UncheckedTransferResultCheck>("fairmq-bugprone-unchecked-transfer-result");
        factories.registerCheck<UseAfterSendCheck>("fairmq-bugprone-use-after-send");
        factories.registerCheck<ChannelsAccessCheck>("fairmq-modernize-channels-access");
        factories.registerCheck<DeprecatedFunctionsCheck>("fairmq-modernize-deprecated-functions");
        factories.registerCheck<DeprecatedHeadersCheck>("fairmq-modernize-deprecated-headers");
        factories.registerCheck<NonNamespacedTypesCheck>("fairmq-modernize-nonnamespaced-types");
        factories.registerCheck<PartsApiCheck>("fairmq-modernize-parts-api");
        factories.registerCheck<ProgOptionsAccessorsCheck>("fairmq-modernize-progoptions-accessors");
        factories.registerCheck<RunDeviceCheck>("fairmq-modernize-run-device");
        factories.registerCheck<TransferCodeCheck>("fairmq-readability-transfer-code");
    }
};

}   // namespace fair::mq::tidy

static clang::tidy::ClangTidyModuleRegistry::Add<fair::mq::tidy::Module>
    fairmqModule("fairmq-module", "Adds the FairMQ checks.");

// A front end that links this module statically references this symbol, so
// the linker keeps the registration above.
// NOLINTNEXTLINE(misc-use-internal-linkage)
volatile int FairMQTidyModuleAnchorSource = 0;
