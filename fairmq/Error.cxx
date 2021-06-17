/********************************************************************************
 * Copyright (C) 2019-2021 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "Error.h"

namespace fair::mq {

const char* ErrorCategory::name() const noexcept { return "fairmq"; }

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
        case ErrorCode::DeviceGetPropertiesFailed:
            return "failed to get fairmq device properties";
        case ErrorCode::DeviceSetPropertiesFailed:
            return "failed to set fairmq device properties";
        default:
            return "(unrecognized error)";
    }
}

const ErrorCategory errorCategory{};

std::error_code MakeErrorCode(ErrorCode e) { return {static_cast<int>(e), errorCategory}; }

}   // namespace fair::mq
