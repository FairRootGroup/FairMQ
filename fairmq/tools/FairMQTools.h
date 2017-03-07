#ifndef FAIRMQTOOLS_H_
#define FAIRMQTOOLS_H_

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
#include <type_traits>
#include <array>

using namespace std;

namespace FairMQ
{
namespace tools
{

// make_unique implementation, until C++14 is default
template<typename T, typename ...Args>
unique_ptr<T> make_unique(Args&& ...args)
{
    return unique_ptr<T>(new T(forward<Args>(args)...));
}

// returns a map with network interface names as keys and their IP addresses as values
int getHostIPs(map<string, string>& addressMap)
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
                cout << "getnameinfo() failed: " << gai_strerror(s) << endl;
                return -1;
            }

            addressMap.insert(pair<string, string>(ifa->ifa_name, host));
        }
    }
    freeifaddrs(ifaddr);

    return 0;
}

// get IP address of a given interface name
string getInterfaceIP(string interface)
{
    map<string, string> IPs;
    FairMQ::tools::getHostIPs(IPs);
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
string getDefaultRouteNetworkInterface()
{
    array<char, 128> buffer;
    string interfaceName;

#ifdef __APPLE__ // MacOS
    unique_ptr<FILE, decltype(pclose) *> file(popen("route -n get default | grep interface | cut -d \":\" -f 2", "r"), pclose);
#else // Linux
    unique_ptr<FILE, decltype(pclose) *> file(popen("ip route | grep default | cut -d \" \" -f 5", "r"), pclose);
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

} // namespace tools
} // namespace FairMQ

#endif // FAIRMQTOOLS_H_
