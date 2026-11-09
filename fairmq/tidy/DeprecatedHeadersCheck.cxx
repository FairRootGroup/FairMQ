/********************************************************************************
 *    Copyright (C) 2026 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <fairmq/tidy/DeprecatedHeadersCheck.h>

#include <clang/Basic/SourceManager.h>
#include <clang/Lex/PPCallbacks.h>
#include <clang/Lex/Preprocessor.h>
#include <llvm/ADT/StringMap.h>
#include <llvm/Config/llvm-config.h>

#include <memory>
#include <string>

namespace fair::mq::tidy {

using namespace clang;

namespace {

/// Deprecated header -> successor. Keep in sync with the
/// `#pragma GCC warning "Deprecated header: ..."` lines of the FairMQ headers.
llvm::StringMap<StringRef> const& successors()
{
    static llvm::StringMap<StringRef> const map{
        {"FairMQChannel.h", "fairmq/Channel.h"},
        {"FairMQDevice.h", "fairmq/Device.h"},
        {"FairMQLogger.h", "fairlogger/Logger.h"},
        {"FairMQMessage.h", "fairmq/Message.h"},
        {"FairMQParts.h", "fairmq/Parts.h"},
        {"FairMQPoller.h", "fairmq/Poller.h"},
        {"FairMQProgOptions.h", "fairmq/ProgOptions.h"},
        {"FairMQSocket.h", "fairmq/Socket.h"},
        {"FairMQTransportFactory.h", "fairmq/TransportFactory.h"},
        {"FairMQUnmanagedRegion.h", "fairmq/UnmanagedRegion.h"},
    };
    return map;
}

class Callbacks : public PPCallbacks
{
  public:
    Callbacks(DeprecatedHeadersCheck& check, SourceManager const& sm)
        : fCheck(check)
        , fSM(sm)
    {}

    void InclusionDirective(SourceLocation /*hashLoc*/,
                            Token const& /*includeTok*/,
                            StringRef fileName,
                            bool isAngled,
                            CharSourceRange fileNameRange,
                            OptionalFileEntryRef /*file*/,
                            StringRef /*searchPath*/,
                            StringRef /*relativePath*/,
#if LLVM_VERSION_MAJOR >= 19
                            Module const* /*suggestedModule*/,
                            bool /*moduleImported*/,
#else
                            Module const* /*imported*/,
#endif
                            SrcMgr::CharacteristicKind /*fileType*/) override
    {
        if (!fSM.isWrittenInMainFile(fileNameRange.getBegin())) {
            return;
        }
        // Match <FairMQDevice.h>, <fairmq/FairMQDevice.h>,
        // <options/FairMQProgOptions.h> and so on.
        auto const it = successors().find(fileName.rsplit('/').second.empty()
                                              ? fileName
                                              : fileName.rsplit('/').second);
        if (it == successors().end()) {
            return;
        }
        std::string const replacement =
            isAngled ? ("<" + it->second + ">").str() : ("\"" + it->second + "\"").str();
        fCheck.diag(fileNameRange.getBegin(), "header '%0' is deprecated; include %1")
            << fileName << replacement
            << FixItHint::CreateReplacement(fileNameRange, replacement);
    }

  private:
    DeprecatedHeadersCheck& fCheck;
    SourceManager const& fSM;
};

}   // namespace

void DeprecatedHeadersCheck::registerPPCallbacks(SourceManager const& sm,
                                                 Preprocessor* pp,
                                                 Preprocessor* /*moduleExpanderPP*/)
{
    pp->addPPCallbacks(std::make_unique<Callbacks>(*this, sm));
}

}   // namespace fair::mq::tidy
