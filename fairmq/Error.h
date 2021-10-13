/********************************************************************************
 *    Copyright (C) 2021 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
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

std::error_code MakeErrorCode(ErrorCode);

struct ErrorCategory : std::error_category
{
    const char* name() const noexcept override;
    std::string message(int ev) const override;
};

}   // namespace fair::mq

namespace std {

template<>
struct is_error_code_enum<fair::mq::ErrorCode> : true_type
{};

}   // namespace std

#endif /* FAIR_MQ_ERROR_H */
