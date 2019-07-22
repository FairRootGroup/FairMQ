/********************************************************************************
 *    Copyright (C) 2019 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_TOOLS_INSTANCELIMIT_H
#define FAIR_MQ_TOOLS_INSTANCELIMIT_H

#include "Strings.h"

namespace fair {
namespace mq {
namespace tools {

template<typename Tag, int Max>
struct InstanceLimiter
{
    InstanceLimiter() { Increment(); }
    explicit InstanceLimiter(const InstanceLimiter&) = delete;
    explicit InstanceLimiter(InstanceLimiter&&) = delete;
    InstanceLimiter& operator=(const InstanceLimiter&) = delete;
    InstanceLimiter& operator=(InstanceLimiter&&) = delete;
    ~InstanceLimiter() { Decrement(); }
    auto GetCount() -> int { return fCount; }

  private:
    auto Increment() -> void
    {
        if (fCount < Max) {
            ++fCount;
        } else {
            throw std::runtime_error(
                ToString("More than ", Max, " instances of ", Tag(), " in parallel not supported"));
        }
    }

    auto Decrement() -> void
    {
        if (fCount > 0) {
            --fCount;
        }
    }

    static int fCount;
};

template<typename Tag, int Max>
int InstanceLimiter<Tag, Max>::fCount(0);

} /* namespace tools */
} /* namespace mq */
} /* namespace fair */

#endif /* FAIR_MQ_TOOLS_INSTANCELIMIT_H */
