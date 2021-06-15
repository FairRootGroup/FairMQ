/********************************************************************************
 * Copyright (C) 2019-2021 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "Topology.h"

#define BOOST_BIND_GLOBAL_PLACEHOLDERS
#include <dds/dds.h>
#undef BOOST_BIND_GLOBAL_PLACEHOLDERS

namespace fair::mq::sdk
{

/// @brief Helper to (Re)Construct a FairMQ topology based on already existing native DDS API objects
/// @param nativeSession Existing and initialized CSession (either via create() or attach())
/// @param nativeTopo Existing CTopology that is activated on the given nativeSession
/// @param env Optional DDSEnv (needed primarily for unit testing)
/// @param blockUntilConnected if true, ctor will wait for all tasks to confirm subscriptions
auto MakeTopology(dds::topology_api::CTopology nativeTopo,
                  std::shared_ptr<dds::tools_api::CSession> nativeSession,
                  DDSEnv env,
                  bool blockUntilConnected) -> Topology
{
    return {DDSTopo(std::move(nativeTopo), env), DDSSession(std::move(nativeSession), env), blockUntilConnected};
}

} // namespace fair::mq::sdk
