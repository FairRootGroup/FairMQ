/********************************************************************************
 *    Copyright (C) 2019 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_SDK_TOPOLOGY_H
#define FAIR_MQ_SDK_TOPOLOGY_H

#include <DDS/Topology.h>
#include <memory>

namespace fair {
namespace mq {
namespace sdk {

/**
 * @class Topology Topology.h <fairmq/sdk/Topology.h>
 * @brief Represents a FairMQ topology
 */
class Topology
{
  public:

    /// Construct a FairMQ topology from an existing DDS session via the dds::topology_api
    /// @param topo An initialized CTopology object
    explicit Topology(dds::topology_api::CTopology topo);

  private:
    dds::topology_api::CTopology fDDSTopology;
};

}   // namespace sdk
}   // namespace mq
}   // namespace fair

#endif /* FAIR_MQ_SDK_TOPOLOGY_H */
