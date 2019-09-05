/********************************************************************************
 *    Copyright (C) 2019 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_SDK_DDSTASK_H
#define FAIR_MQ_SDK_DDSTASK_H

#include <fairmq/sdk/DDSCollection.h>

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

    explicit DDSTask(Id id, Id collectionId)
        : fId(id)
        , fCollectionId(collectionId)
    {}

    Id GetId() const { return fId; }
    DDSCollection::Id GetCollectionId() const { return fCollectionId; }

    friend auto operator<<(std::ostream& os, const DDSTask& task) -> std::ostream&
    {
        return os << "DDSTask id: " << task.fId << ", collection id: " << task.fCollectionId;
    }

  private:
    Id fId;
    DDSCollection::Id fCollectionId;
};

}   // namespace sdk
}   // namespace mq
}   // namespace fair

#endif /* FAIR_MQ_SDK_DDSTASK_H */
