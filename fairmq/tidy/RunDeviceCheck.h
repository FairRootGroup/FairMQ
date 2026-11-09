/********************************************************************************
 *    Copyright (C) 2026 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_TIDY_RUNDEVICECHECK_H
#define FAIR_MQ_TIDY_RUNDEVICECHECK_H

#include <clang-tidy/ClangTidyCheck.h>

#include <optional>

namespace fair::mq::tidy {

/// Migrates a device main file from `<runFairMQDevice.h>` to
/// `<fairmq/runDevice.h>`:
///
///     #include <runFairMQDevice.h>               #include <fairmq/runDevice.h>
///     FairMQDevicePtr                      ->    std::unique_ptr<fair::mq::Device>
///     getDevice(const ProgOptions& cfg)          getDevice(fair::mq::ProgOptions& cfg)
///     { return new MyDevice(cfg); }              { return std::make_unique<MyDevice>(cfg); }
///
/// The check rewrites all of it in one fix, or nothing. It reports without a
/// fix if a return statement is not a plain `new` expression.
class RunDeviceCheck : public clang::tidy::ClangTidyCheck
{
  public:
    using ClangTidyCheck::ClangTidyCheck;

    bool isLanguageVersionSupported(clang::LangOptions const& opts) const override
    {
        return opts.CPlusPlus17;
    }
    void registerPPCallbacks(clang::SourceManager const& sm,
                             clang::Preprocessor* pp,
                             clang::Preprocessor* moduleExpanderPP) override;
    void registerMatchers(clang::ast_matchers::MatchFinder* finder) override;
    void check(clang::ast_matchers::MatchFinder::MatchResult const& result) override;

    struct Include
    {
        clang::CharSourceRange fileNameRange;
        bool isAngled;
    };

  private:
    std::optional<Include> fInclude;   // #include <runFairMQDevice.h> in the main file
};

}   // namespace fair::mq::tidy

#endif /* FAIR_MQ_TIDY_RUNDEVICECHECK_H */
