/********************************************************************************
 * Copyright (C) 2017-2021 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_TOOLS_NETWORK_H
#define FAIR_MQ_TOOLS_NETWORK_H

#include <map>
#include <string>
#include <stdexcept>

namespace fair::mq::tools
{

struct DefaultRouteDetectionError : std::runtime_error { using std::runtime_error::runtime_error; };

// returns a map with network interface names as keys and their IP addresses as values
std::map<std::string, std::string> getHostIPs();

// get IP address of a given interface name
std::string getInterfaceIP(const std::string& interface);

// get name of the default route interface
std::string getDefaultRouteNetworkInterface();

std::string getIpFromHostname(const std::string& hostname);

} // namespace fair::mq::tools

#endif /* FAIR_MQ_TOOLS_NETWORK_H */
