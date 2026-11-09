/********************************************************************************
 * Copyright (C) 2021-2026 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

// fairmq-tidy is the upstream clang-tidy driver with the FairMQ module linked in.
// It accepts every clang-tidy option and runs the upstream checks and the
// fairmq-* checks on one parse of each file. Without a --checks option it adds
// the fairmq-* checks to the checks of the .clang-tidy configuration.

#include <clang-tidy/tool/ClangTidyMain.h>

#include <string_view>
#include <vector>

extern volatile int FairMQTidyModuleAnchorSource;
[[maybe_unused]] static int const fairmqModuleAnchor = FairMQTidyModuleAnchorSource;

int main(int argc, char const** argv)
{
    std::vector<char const*> args(argv, argv + argc);
    bool userResourceDir = false;
    bool userChecks = false;
    for (std::string_view arg : args) {
        if (arg == "--") {
            break;   // the compiler command line follows
        }
        if (arg.find("-resource-dir") != std::string_view::npos) {
            userResourceDir = true;
        }
        if (arg.rfind("--checks", 0) == 0 || arg.rfind("-checks", 0) == 0) {
            userChecks = true;
        }
    }
    // The driver looks for its builtin headers (e.g. <stddef.h>) relative to
    // the executable. fairmq-tidy is not installed next to clang, so point it
    // to the resource directory of the LLVM installation it was built with.
    if (argc > 0 && !userResourceDir) {
        args.insert(args.begin() + 1, "--extra-arg-before=-resource-dir=" FAIRMQ_TIDY_CLANG_RESOURCE_DIR);
    }
    // --checks appends to the Checks of the configuration.
    if (argc > 0 && !userChecks) {
        args.insert(args.begin() + 1, "--checks=fairmq-*");
    }

    return clang::tidy::clangTidyMain(static_cast<int>(args.size()), args.data());
}
