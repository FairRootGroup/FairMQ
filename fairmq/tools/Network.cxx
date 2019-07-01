/********************************************************************************
 *    Copyright (C) 2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <fairmq/tools/Network.h>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE // To get defns of NI_MAXSERV and NI_MAXHOST
#endif

#include <fairlogger/Logger.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <stdio.h>

#include <boost/algorithm/string.hpp> // trim
#include <boost/asio.hpp>

#include <map>
#include <string>
#include <iostream>
#include <array>
#include <exception>
#include <stdexcept>
#include <algorithm>

using namespace std;

namespace fair
{
namespace mq
{
namespace tools
{

// returns a map with network interface names as keys and their IP addresses as values
map<string, string> getHostIPs()
{
    map<string, string> addressMap;
    struct ifaddrs *ifaddr, *ifa;
    int s;
    char host[NI_MAXHOST];

    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        throw runtime_error("getifaddrs failed");
    }

    for (ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == nullptr) {
            continue;
        }

        if (ifa->ifa_addr->sa_family == AF_INET) {
            s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), host, NI_MAXHOST, nullptr, 0, NI_NUMERICHOST);
            if (s != 0) {
                cout << "getnameinfo() failed: " << gai_strerror(s) << endl;
                throw runtime_error("getnameinfo() failed");
            }

            addressMap.insert(pair<string, string>(ifa->ifa_name, host));
        }
    }

    freeifaddrs(ifaddr);

    return addressMap;
}

// get IP address of a given interface name
string getInterfaceIP(const string& interface)
{
    try {
        auto IPs = getHostIPs();
        if (IPs.count(interface)) {
            return IPs[interface];
        } else {
            LOG(error) << "Could not find provided network interface: \"" << interface << "\"!, exiting.";
            return "";
        }
    } catch (runtime_error& re) {
        cout << "could not get interface IP: " << re.what();
        return "";
    }
}

// get name of the default route interface
string getDefaultRouteNetworkInterface()
{
    array<char, 128> buffer;
    string interfaceName;

#ifdef __APPLE__ // MacOS
    unique_ptr<FILE, decltype(pclose) *> file(popen("route -n get default | grep interface | cut -d \":\" -f 2", "r"), pclose);
#else // Linux
    unique_ptr<FILE, decltype(pclose) *> file(popen("ip route | grep default | cut -d \" \" -f 5 | head -n 1", "r"), pclose);
#endif

    if (!file) {
        LOG(error) << "Could not detect default route network interface name - popen() failed!";
        return "";
    }

    while (!feof(file.get())) {
        if (fgets(buffer.data(), 128, file.get()) != nullptr) {
            interfaceName += buffer.data();
        }
    }

    boost::algorithm::trim(interfaceName);

    if (interfaceName == "") {
        LOG(error) << "Could not detect default route network interface name";
    } else {
        LOG(debug) << "Detected network interface name for the default route: " << interfaceName;
    }

    return interfaceName;
}

string getIpFromHostname(const string& hostname)
{
    boost::asio::io_service ios;
    return getIpFromHostname(hostname, ios);
}

string getIpFromHostname(const string& hostname, boost::asio::io_service& ios)
{
    try {
        namespace bai = boost::asio::ip;
        bai::tcp::resolver resolver(ios);
        bai::tcp::resolver::query query(hostname, "");
        bai::tcp::resolver::iterator end;

        auto it = find_if(static_cast<bai::basic_resolver_iterator<bai::tcp>>(resolver.resolve(query)), end, [](const bai::tcp::endpoint& ep) {
            return ep.address().is_v4();
        });

        if (it != end) {
            stringstream ss;
            ss << static_cast<bai::tcp::endpoint>(*it).address();
            return ss.str();
        }

        LOG(warn) << "could not find ipv4 address for hostname '" << hostname << "'";

        return "";
    } catch (exception& e) {
        LOG(error) << "could not resolve hostname '" << hostname << "', reason: " << e.what();
        return "";
    }
}

} /* namespace tools */
} /* namespace mq */
} /* namespace fair */
