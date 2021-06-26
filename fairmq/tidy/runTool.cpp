/********************************************************************************
 *    Copyright (C) 2021 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>
#include <fairmq/tidy/Tool.h>
#include <llvm/ADT/STLExtras.h>
#include <llvm/Support/CommandLine.h>

static llvm::cl::OptionCategory ToolCategory("fairmq-tidy options");
static llvm::cl::extrahelp CommonHelp(clang::tooling::CommonOptionsParser::HelpMessage);

int main(int argc, const char** argv)
{
    // TODO Replace command line parser with CLI11
    auto parser(clang::tooling::CommonOptionsParser::create(
        argc, argv, ToolCategory, llvm::cl::NumOccurrencesFlag::ZeroOrMore, ""));
    if (!parser) {
        llvm::errs() << parser.takeError();
        return EXIT_FAILURE;
    }

    return fair::mq::tidy::Tool::run(parser->getCompilations(), parser->getSourcePathList());
}
