/********************************************************************************
 *    Copyright (C) 2019 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "TopologyFixture.h"

#include <fairmq/sdk/Topology.h>

namespace {

using Topology = fair::mq::test::TopologyFixture;

TEST_F(Topology, Basic)
{
  fair::mq::sdk::Topology topo;
}

}   // namespace
