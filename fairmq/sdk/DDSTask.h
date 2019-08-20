/********************************************************************************
 *    Copyright (C) 2019 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_SDK_DDSTASK_H
#define FAIR_MQ_SDK_DDSTASK_H

// #include <fairmq/sdk/DDSAgent.h>

#include <ostream>
#include <cstdint>

namespace fair {
namespace mq {
namespace sdk {

/**
 * @class DDSTask <fairmq/sdk/DDSTask.h>
 * @brief Represents a DDS task
 */
class DDSTask
{
  public:
    using Id = std::uint64_t;

    explicit DDSTask(Id id)
        : fId(id)
    {}

    Id GetId() const { return fId; }

    friend auto operator<<(std::ostream& os, const DDSTask& task) -> std::ostream&
    {
        return os << "DDSTask id: " << task.fId;
    }

  private:
    Id fId;
};

}   // namespace sdk
}   // namespace mq
}   // namespace fair

#endif /* FAIR_MQ_SDK_DDSTASK_H */
