/********************************************************************************
 *    Copyright (C) 2021 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_TIDY_TOOL
#define FAIR_MQ_TIDY_TOOL

#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/Basic/FileManager.h>
#include <clang/Tooling/CompilationDatabase.h>
#include <clang/Tooling/Tooling.h>
#include <fairmq/tidy/ModernizeNonNamespacedTypes.h>
#include <string>

namespace fair::mq::tidy {

// Getting up to speed, read this:
// https://clang.llvm.org/docs/LibTooling.html
// https://clang.llvm.org/docs/IntroductionToTheClangAST.html
// Watch https://www.youtube.com/watch?v=VqCkCDFLSsc !!!
//
// optional, but helpful:
// https://static.linaro.org/connect/yvr18/presentations/yvr18-223.pdf
// https://steveire.wordpress.com/2018/11/20/composing-ast-matchers-in-clang-tidy/
// https://www.goldsborough.me/c++/clang/llvm/tools/2017/02/24/00-00-06-emitting_diagnostics_and_fixithints_in_clang_tools/
// https://steveire.com/CodeDive2018Presentation.pdf
//
// reference (no built-in search, however google will find things):
// https://clang.llvm.org/doxygen/
//
// Note: Implementing a standalone tool will impose double PP and parsing cost if one also
// runs clang-tidy. At the moment, one cannot extend clang-tidy with new checks without
// recompiling clang-tidy. In principle llvm-project/clang-extra-tools/clang-tidy
// has install rules, but Fedora is not packaging them which is likely due to their
// unstable state (lots of comments aka todo, fixme, refactor etc). Also, it seems
// focus has shifted towards implementing the
// https://microsoft.github.io/language-server-protocol/
// via https://clangd.llvm.org/ which includes clang-tidy, but also does not have
// an extension/plugin interface for third-party checks yet AFAICS.

struct Tool
{
    static auto run(clang::tooling::CompilationDatabase const &compilations,
                    clang::ArrayRef<std::string> sources) -> int
    {
        using namespace clang;

        // compose all checks in a single match finder
        ast_matchers::MatchFinder finder;
        ModernizeNonNamespacedTypes check1(finder);

        // configure the clang tool
        tooling::ClangTool tool(compilations, sources);
        tool.appendArgumentsAdjuster(
            [](tooling::CommandLineArguments const &_args, StringRef /*file*/) {
                tooling::CommandLineArguments args(_args);
                // TODO add only if cdb was generated with GCC
                args.emplace(args.begin() + 1, "-I/usr/lib64/clang/12.0.0/include");
                // TODO add only if missing
                args.emplace(args.begin() + 1, "-std=c++17");
                args.emplace_back("-Wno-everything");
                return args;
            });

        // run checks on given files
        return tool.run(clang::tooling::newFrontendActionFactory(&finder).get());
    }
};

}   // namespace fair::mq::tidy

#endif /* FAIR_MQ_TIDY_TOOL */
