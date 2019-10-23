/********************************************************************************
 *    Copyright (C) 2019 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_SDK_ERROR_H
#define FAIR_MQ_SDK_ERROR_H

#include <fairmq/tools/Strings.h>
#include <stdexcept>
#include <system_error>

namespace fair {
namespace mq {
namespace sdk {

struct RuntimeError : ::std::runtime_error
{
    template<typename... T>
    explicit RuntimeError(T&&... t)
        : ::std::runtime_error::runtime_error(tools::ToString(std::forward<T>(t)...))
    {}
};

struct MixedStateError : RuntimeError
{
    using RuntimeError::RuntimeError;
};

} /* namespace sdk */

enum class ErrorCode
{
    OperationInProgress = 10,
    OperationTimeout,
    OperationCanceled,
    DeviceChangeStateFailed
};

std::error_code MakeErrorCode(ErrorCode);

struct ErrorCategory : std::error_category
{
  const char* name() const noexcept override;
  std::string message(int ev) const override;
};

} /* namespace mq */
} /* namespace fair */

namespace std {

template<>
struct is_error_code_enum<fair::mq::ErrorCode> : true_type
{};

}   // namespace std

#endif /* FAIR_MQ_SDK_ERROR_H */
