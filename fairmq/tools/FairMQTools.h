#ifndef FAIRMQTOOLS_H_
#define FAIRMQTOOLS_H_

#ifndef _GNU_SOURCE
#define _GNU_SOURCE // To get defns of NI_MAXSERV and NI_MAXHOST
#endif

#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <stdio.h>

#include <map>
#include <string>
#include <iostream>
#include <type_traits>

using namespace std;

namespace FairMQ
{
namespace tools
{

template<typename T, typename ...Args>
unique_ptr<T> make_unique(Args&& ...args)
{
    return unique_ptr<T>(new T(forward<Args>(args)...));
}

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
        exit(EXIT_FAILURE);
    }
}

#if defined(__GNUC__) || defined(__GNUG__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
#endif
// below are SFINAE template functions to check for function member signatures of class
namespace details
{
// test, at compile time, whether T has BindSendHeader member function with returned type R and argument ...Args type
template<class T, class Sig, class=void>
struct has_BindSendHeader : false_type {};

template<class T, class R, class... Args>
struct has_BindSendHeader
    <T, R(Args...), typename enable_if<
        is_convertible<decltype(declval<T>().BindSendHeader(declval<Args>()...)), R>::value || is_same<R, void>::value>::type
    >:true_type {};

// test, at compile time, whether T has BindGetSocketNumber member function with returned type R and argument ...Args type
template<class T, class Sig, class=void>
struct has_BindGetSocketNumber : false_type {};

template<class T, class R, class... Args>
struct has_BindGetSocketNumber
    <T, R(Args...), typename enable_if<
        is_convertible<decltype(declval<T>().BindGetSocketNumber(declval<Args>()...)), R>::value || is_same<R, void>::value>::type
    >:true_type {};

// test, at compile time, whether T has GetHeader member function with returned type R and argument ...Args type
template<class T, class Sig, class=void>
struct has_GetHeader : false_type {};

template<class T, class R, class... Args>
struct has_GetHeader
    <T, R(Args...), typename enable_if<
        is_convertible<decltype(declval<T>().GetHeader(declval<Args>()...)), R>::value || is_same<R, void>::value>::type
    >:true_type {};

// test, at compile time, whether T has BindGetCurrentIndex member function with returned type R and argument ...Args type
template<class T, class Sig, class=void>
struct has_BindGetCurrentIndex : false_type {};

template<class T, class R, class... Args>
struct has_BindGetCurrentIndex
    <T, R(Args...), typename enable_if<
        is_convertible<decltype(declval<T>().BindGetCurrentIndex(declval<Args>()...)), R>::value || is_same<R, void>::value>::type
    >:true_type {};



} // end namespace details
#if defined(__GNUC__) || defined(__GNUG__)
#pragma GCC diagnostic pop
#endif

// Alias template of the above structs
template<class T, class Sig>
using has_BindSendHeader = integral_constant<bool, details::has_BindSendHeader<T, Sig>::value>;

template<class T, class Sig>
using has_BindGetSocketNumber = integral_constant<bool, details::has_BindGetSocketNumber<T, Sig>::value>;

template<class T, class Sig>
using has_GetHeader = integral_constant<bool, details::has_GetHeader<T, Sig>::value>;

template<class T, class Sig>
using has_BindGetCurrentIndex = integral_constant<bool, details::has_BindGetCurrentIndex<T, Sig>::value>;

// enable_if Alias template
template<typename T>
using enable_if_has_BindSendHeader = typename enable_if<has_BindSendHeader<T, void(int)>::value, int>::type;
template<typename T>
using enable_if_hasNot_BindSendHeader = typename enable_if<!has_BindSendHeader<T, void(int)>::value, int>::type;

template<typename T>
using enable_if_has_BindGetSocketNumber = typename enable_if<has_BindGetSocketNumber<T, int()>::value, int>::type;
template<typename T>
using enable_if_hasNot_BindGetSocketNumber = typename enable_if<!has_BindGetSocketNumber<T, int()>::value, int>::type;

template<typename T>
using enable_if_has_BindGetCurrentIndex = typename enable_if<has_BindGetCurrentIndex<T, int()>::value, int>::type;
template<typename T>
using enable_if_hasNot_BindGetCurrentIndex = typename enable_if<!has_BindGetCurrentIndex<T, int()>::value, int>::type;

template<typename T>
using enable_if_has_GetHeader = typename enable_if<has_GetHeader<T, int()>::value, int>::type;
template<typename T>
using enable_if_hasNot_GetHeader = typename enable_if<!has_GetHeader<T, int()>::value, int>::type;

} // namespace tools
} // namespace FairMQ

#endif
