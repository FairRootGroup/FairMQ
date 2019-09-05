/********************************************************************************
 *    Copyright (C) 2019 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_SDK_DDSCOLLECTION_H
#define FAIR_MQ_SDK_DDSCOLLECTION_H

// #include <fairmq/sdk/DDSAgent.h>

#include <ostream>
#include <cstdint>

namespace fair {
namespace mq {
namespace sdk {

/**
 * @class DDSCollection <fairmq/sdk/DDSCollection.h>
 * @brief Represents a DDS collection
 */
class DDSCollection
{
  public:
    using Id = std::uint64_t;

    explicit DDSCollection(Id id)
        : fId(id)
    {}

    Id GetId() const { return fId; }

    friend auto operator<<(std::ostream& os, const DDSCollection& collection) -> std::ostream&
    {
        return os << "DDSCollection id: " << collection.fId;
    }

  private:
    Id fId;
};

}   // namespace sdk
}   // namespace mq
}   // namespace fair

#endif /* FAIR_MQ_SDK_DDSCOLLECTION_H */
