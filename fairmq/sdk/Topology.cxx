/********************************************************************************
 *    Copyright (C) 2019 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "Topology.h"

#include <DDS/Topology.h>
#include <utility>

namespace fair {
namespace mq {
namespace sdk {

struct Topology::Impl
{
    Impl(dds::topology_api::CTopology topo)
        : fDDSTopology(std::move(topo))
    {}

    dds::topology_api::CTopology fDDSTopology;
};

Topology::Topology(dds::topology_api::CTopology topo)
    : fImpl(std::make_shared<Impl>(std::move(topo)))
{}

}   // namespace sdk
}   // namespace mq
}   // namespace fair
