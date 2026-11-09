/********************************************************************************
 *    Copyright (C) 2026 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

// fairmq-clangd is the upstream clangd language server with the FairMQ module
// linked in. clangd cannot load clang-tidy plugins, but it creates its
// clang-tidy checks from the same registry as clang-tidy. Thus the fairmq-*
// checks run on the AST that clangd builds anyway, and their fixes show as
// quick fixes in the editor.
//
// Enable the checks in a .clangd file:
//
//   Diagnostics:
//     ClangTidy:
//       Add: ["fairmq-*"]
//       FastCheckFilter: Loose   # clangd >= 18 skips checks of unknown cost

#include <string>
#include <string_view>
#include <vector>

namespace clang::clangd {
// Defined in the clangdMain library. The header ClangdMain.h is not installed.
int clangdMain(int argc, char* argv[]);
}   // namespace clang::clangd

extern volatile int FairMQTidyModuleAnchorSource;
[[maybe_unused]] static int const fairmqModuleAnchor = FairMQTidyModuleAnchorSource;

int main(int argc, char* argv[])
{
    // clangd looks for the Clang builtin headers relative to its executable.
    // Point it to the LLVM installation it was built with, unless the user
    // gives a resource directory.
    std::vector<char*> args(argv, argv + argc);
    bool userResourceDir = false;
    for (std::string_view arg : args) {
        if (arg.rfind("--resource-dir", 0) == 0 || arg.rfind("-resource-dir", 0) == 0) {
            userResourceDir = true;
        }
    }
    std::string resourceDirArg = "--resource-dir=" FAIRMQ_TIDY_CLANG_RESOURCE_DIR;
    if (!userResourceDir && argc > 0) {
        args.insert(args.begin() + 1, resourceDirArg.data());
    }
    auto const count = static_cast<int>(args.size());
    args.push_back(nullptr);   // argv[argc] is a null pointer

    return clang::clangd::clangdMain(count, args.data());
}
