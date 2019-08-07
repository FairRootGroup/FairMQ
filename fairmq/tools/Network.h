/********************************************************************************
 *    Copyright (C) 2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_TOOLS_NETWORK_H
#define FAIR_MQ_TOOLS_NETWORK_H

#include <map>
#include <string>

// forward declarations
namespace boost
{
namespace asio
{

#ifdef FAIR_MQ_HAS_NO_ASIO_IO_CONTEXT
class io_service;
#else
class io_context;
typedef class io_context io_service;
#endif

} // namespace asio
} // namespace boost

namespace fair
{
namespace mq
{
namespace tools
{

// returns a map with network interface names as keys and their IP addresses as values
std::map<std::string, std::string> getHostIPs();

// get IP address of a given interface name
std::string getInterfaceIP(const std::string& interface);

// get name of the default route interface
std::string getDefaultRouteNetworkInterface();

std::string getIpFromHostname(const std::string& hostname);

std::string getIpFromHostname(const std::string& hostname, boost::asio::io_service& ios);

} /* namespace tools */
} /* namespace mq */
} /* namespace fair */

#endif /* FAIR_MQ_TOOLS_NETWORK_H */
