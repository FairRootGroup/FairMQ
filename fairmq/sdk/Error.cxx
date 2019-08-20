/********************************************************************************
 *    Copyright (C) 2019 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "Error.h"

namespace fair {
namespace mq {

const char* ErrorCategory::name() const noexcept
{
    return "fairmq";
}

std::string ErrorCategory::message(int ev) const
{
    switch (static_cast<ErrorCode>(ev)) {
        case ErrorCode::OperationInProgress:
            return "async operation already in progress";
        case ErrorCode::OperationTimeout:
            return "async operation timed out";
        case ErrorCode::OperationCanceled:
            return "async operation canceled";
        case ErrorCode::DeviceChangeStateFailed:
            return "failed to change state of a fairmq device";
        default:
            return "(unrecognized error)";
    }
}

const ErrorCategory errorCategory{};

std::error_code MakeErrorCode(ErrorCode e) { return {static_cast<int>(e), errorCategory}; }

}   // namespace mq
}   // namespace fair
