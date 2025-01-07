/********************************************************************************
 * Copyright (C) 2017-2025 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <fairlogger/Logger.h>
#include <fairmq/tools/Network.h>
#include <fairmq/tools/Strings.h>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE   // To get defns of NI_MAXSERV and NI_MAXHOST
#endif

#include <array>
#include <boost/algorithm/string.hpp>   // trim
#include <boost/asio.hpp>
#include <cstdio>
#include <exception>
#include <fstream>
#include <ifaddrs.h>
#include <iostream>
#include <map>
#include <netdb.h>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>

using namespace std;

namespace fair::mq::tools {

// returns a map with network interface names as keys and their IP addresses as values
map<string, string> getHostIPs()
{
    map<string, string> addressMap;
    ifaddrs* ifaddr = nullptr;
    ifaddrs* ifa = nullptr;
    int s = 0;
    array<char, NI_MAXHOST> host{};

    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        throw runtime_error("getifaddrs failed");
    }

    for (ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == nullptr) {
            continue;
        }

        if (ifa->ifa_addr->sa_family == AF_INET) {
            s = getnameinfo(ifa->ifa_addr,
                            sizeof(sockaddr_in),
                            host.data(),
                            NI_MAXHOST,
                            nullptr,
                            0,
                            NI_NUMERICHOST);
            if (s != 0) {
                cout << "getnameinfo() failed: " << gai_strerror(s) << endl;
                throw runtime_error("getnameinfo() failed");
            }

            addressMap.insert({ifa->ifa_name, host.data()});
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
        if (IPs.count(interface) > 0) {
            return IPs[interface];
        }
        LOG(error) << "Could not find provided network interface: \""
                   << interface << "\"!, exiting.";
        return "";
    } catch (runtime_error& re) {
        cout << "could not get interface IP: " << re.what();
        return "";
    }
}

// get name of the default route interface
string getDefaultRouteNetworkInterface()
{
    const int BUFSIZE(128);
    array<char, BUFSIZE> buffer{};
    string interfaceName;

#ifdef __APPLE__   // MacOS
    unique_ptr<FILE, decltype(pclose)*> file(
        popen("route -n get default | grep interface | cut -d \":\" -f 2", "r"), pclose);
#else   // Linux
    ifstream is("/proc/net/route");
    string line;

    // discard header
    getline(is, line);

    // check each line, until 00000000 destination is found
    while (!is.eof()) {
        getline(is, line);
        size_t pos = line.find('\t');

        if (pos == string::npos) {
            break;
        }

        if (line.substr(pos + 1, 8) == "00000000") {
            interfaceName = line.substr(0, pos);
            LOG(debug) << "Detected network interface name for the default route: "
                       << interfaceName;
            return interfaceName;
        }
    }

    LOG(debug) << "could not get network interface of the default route from /proc/net/route, "
                  "going to try via 'ip route'";

    unique_ptr<FILE, decltype(pclose)*> file(
        popen("ip route | grep default | cut -d \" \" -f 5 | head -n 1", "r"), pclose);
#endif

    if (!file) {
        LOG(error) << "Could not detect default route network interface name - popen() failed!";
        return "";
    }

    while (feof(file.get()) == 0) {
        if (fgets(buffer.data(), BUFSIZE, file.get()) != nullptr) {
            interfaceName += buffer.data();
        }
    }

    boost::algorithm::trim(interfaceName);

    if (interfaceName.empty()) {
        LOG(debug) << "Could not detect default route network interface name from /proc/net/route "
                      "nor 'ip route'";
        throw DefaultRouteDetectionError("Could not detect default route network interface name "
                                         "from /proc/net/route nor 'ip route'");
    } else {
        LOG(debug) << "Detected network interface name for the default route: " << interfaceName;
    }

    return interfaceName;
}

string getIpFromHostname(const string& hostname)
try {
    boost::asio::io_context ioc;
    boost::asio::ip::tcp::resolver resolver(ioc);

    auto const result = resolver.resolve(boost::asio::ip::tcp::v4(), hostname, "");

    if (result.empty()) {
        LOG(warn) << "could not find ipv4 address for hostname '" << hostname << "'";
        return "";
    }
    return ToString(result.begin()->endpoint().address());
}
catch (std::exception const& ex)
{
    LOG(error) << "could not resolve hostname '" << hostname << "', reason: " << ex.what();
    return "";
}

}   // namespace fair::mq::tools
