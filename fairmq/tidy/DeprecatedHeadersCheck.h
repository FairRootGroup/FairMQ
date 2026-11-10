/********************************************************************************
 *    Copyright (C) 2026 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_TIDY_DEPRECATEDHEADERSCHECK_H
#define FAIR_MQ_TIDY_DEPRECATEDHEADERSCHECK_H

#include <clang-tidy/ClangTidyCheck.h>

namespace fair::mq::tidy {

/// Finds includes of the deprecated top-level headers (e.g. `<FairMQDevice.h>`)
/// and replaces them with their successor (e.g. `<fairmq/Device.h>`).
class DeprecatedHeadersCheck : public clang::tidy::ClangTidyCheck
{
  public:
    using ClangTidyCheck::ClangTidyCheck;

    bool isLanguageVersionSupported(clang::LangOptions const& opts) const override
    {
        return opts.CPlusPlus;
    }
    void registerPPCallbacks(clang::SourceManager const& sm,
                             clang::Preprocessor* pp,
                             clang::Preprocessor* moduleExpanderPP) override;
};

}   // namespace fair::mq::tidy

#endif /* FAIR_MQ_TIDY_DEPRECATEDHEADERSCHECK_H */
