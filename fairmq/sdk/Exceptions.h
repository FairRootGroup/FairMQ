/********************************************************************************
 *    Copyright (C) 2019 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_SDK_EXCEPTIONS_H
#define FAIR_MQ_SDK_EXCEPTIONS_H

#include <fairmq/Tools.h>
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
} /* namespace mq */
} /* namespace fair */

#endif /* FAIR_MQ_SDK_EXCEPTIONS_H */
