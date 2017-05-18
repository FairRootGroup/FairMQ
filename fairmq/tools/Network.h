/********************************************************************************
 *    Copyright (C) 2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_TOOLS_NETWORK_H
#define FAIR_MQ_TOOLS_NETWORK_H

#ifndef _GNU_SOURCE
#define _GNU_SOURCE // To get defns of NI_MAXSERV and NI_MAXHOST
#endif

#include "FairMQLogger.h"

#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <stdio.h>

#include <boost/algorithm/string.hpp> // trim

#include <map>
#include <string>
#include <iostream>
#include <array>

namespace fair
{
namespace mq
{
namespace tools
{

// returns a map with network interface names as keys and their IP addresses as values
int getHostIPs(std::map<std::string, std::string>& addressMap)
{
    struct ifaddrs *ifaddr, *ifa;
    int s;
    char host[NI_MAXHOST];

    if (getifaddrs(&ifaddr) == -1)
    {
        perror("getifaddrs");
        return -1;
    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr == NULL)
        {
            continue;
        }

        if (ifa->ifa_addr->sa_family == AF_INET)
        {
            s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
            if (s != 0)
            {
                std::cout << "getnameinfo() failed: " << gai_strerror(s) << std::endl;
                return -1;
            }

            addressMap.insert(std::pair<std::string, std::string>(ifa->ifa_name, host));
        }
    }
    freeifaddrs(ifaddr);

    return 0;
}

// get IP address of a given interface name
std::string getInterfaceIP(std::string interface)
{
    std::map<std::string, std::string> IPs;
    getHostIPs(IPs);
    if (IPs.count(interface))
    {
        return IPs[interface];
    }
    else
    {
        LOG(ERROR) << "Could not find provided network interface: \"" << interface << "\"!, exiting.";
        return "";
    }
}

// get name of the default route interface
std::string getDefaultRouteNetworkInterface()
{
    std::array<char, 128> buffer;
    std::string interfaceName;

#ifdef __APPLE__ // MacOS
    std::unique_ptr<FILE, decltype(pclose) *> file(popen("route -n get default | grep interface | cut -d \":\" -f 2", "r"), pclose);
#else // Linux
    std::unique_ptr<FILE, decltype(pclose) *> file(popen("ip route | grep default | cut -d \" \" -f 5", "r"), pclose);
#endif

    if (!file)
    {
        LOG(ERROR) << "Could not detect default route network interface name - popen() failed!";
        return "";
    }

    while (!feof(file.get()))
    {
        if (fgets(buffer.data(), 128, file.get()) != NULL)
        {
            interfaceName += buffer.data();
        }
    }

    boost::algorithm::trim(interfaceName);

    if (interfaceName == "")
    {
        LOG(ERROR) << "Could not detect default route network interface name";
    }
    else
    {
        LOG(DEBUG) << "Detected network interface name for the default route: " << interfaceName;
    }

    return interfaceName;
}

} /* namespace tools */
} /* namespace mq */
} /* namespace fair */

#endif /* FAIR_MQ_TOOLS_NETWORK_H */
