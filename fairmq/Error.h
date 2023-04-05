/********************************************************************************
 * Copyright (C) 2021-2023 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_ERROR_H
#define FAIR_MQ_ERROR_H

#include <cassert>
#include <fairmq/tools/Strings.h>
#include <stdexcept>
#include <system_error>

// Macro copied from https://en.cppreference.com/w/cpp/error/assert
// Use (void) to silent unused warnings.
#define assertm(exp, msg) assert(((void)msg, exp))

namespace fair::mq {

struct RuntimeError : ::std::runtime_error
{
    template<typename... T>
    explicit RuntimeError(T&&... t)
        : ::std::runtime_error::runtime_error(tools::ToString(std::forward<T>(t)...))
    {}
};

enum class ErrorCode
{
    OperationInProgress = 10,
    OperationTimeout,
    OperationCanceled,
    DeviceChangeStateFailed,
    DeviceGetPropertiesFailed,
    DeviceSetPropertiesFailed
};

struct ErrorCategory : std::error_category
{
    const char* name() const noexcept override { return "fairmq"; }
    std::string message(int ev) const override
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
};

static ErrorCategory ec;

inline std::error_code MakeErrorCode(ErrorCode e) { return {static_cast<int>(e), ec}; }

}   // namespace fair::mq

namespace std {

template<>
struct is_error_code_enum<fair::mq::ErrorCode> : true_type
{};

}   // namespace std

#endif /* FAIR_MQ_ERROR_H */
