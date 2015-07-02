#ifndef FAIRMQTOOLS_H_
#define FAIRMQTOOLS_H_

#ifndef _GNU_SOURCE
#define _GNU_SOURCE     /* To get defns of NI_MAXSERV and NI_MAXHOST */
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



// below are SFINAE template functions to check for function member signatures of class
namespace details 
{
    ///////////////////////////////////////////////////////////////////////////
    // test, at compile time, whether T has BindSendPart member function with returned type R and argument ...Args type
    template<class T, class Sig, class=void>
    struct has_BindSendPart:std::false_type{};

    template<class T, class R, class... Args>
    struct has_BindSendPart
        <T, R(Args...),
            typename std::enable_if
            <
                std::is_convertible< decltype(std::declval<T>().BindSendPart(std::declval<Args>()...)), R >::value
                || std::is_same<R, void>::value 
            >::type
        >:std::true_type{};

    ///////////////////////////////////////////////////////////////////////////
    // test, at compile time, whether T has BindGetSocketNumber member function with returned type R and argument ...Args type
    template<class T, class Sig, class=void>
    struct has_BindGetSocketNumber:std::false_type{};

    template<class T, class R, class... Args>
    struct has_BindGetSocketNumber
        <T, R(Args...),
            typename std::enable_if
            <
                std::is_convertible< decltype(std::declval<T>().BindGetSocketNumber(std::declval<Args>()...)), R >::value
                || std::is_same<R, void>::value 
            >::type
        >:std::true_type{};
        
    ///////////////////////////////////////////////////////////////////////////
    // test, at compile time, whether T has BindGetCurrentIndex member function with returned type R and argument ...Args type
    template<class T, class Sig, class=void>
    struct has_BindGetCurrentIndex:std::false_type{};

    template<class T, class R, class... Args>
    struct has_BindGetCurrentIndex
        <T, R(Args...),
            typename std::enable_if
            <
                std::is_convertible< decltype(std::declval<T>().BindGetCurrentIndex(std::declval<Args>()...)), R >::value
                || std::is_same<R, void>::value 
            >::type
        >:std::true_type{};

}// end namespace details

///////////////////////////////////////////////////////////////////////////
// Alias template of the above structs
template<class T, class Sig>
using has_BindSendPart = std::integral_constant<bool, details::has_BindSendPart<T, Sig>::value>;

template<class T, class Sig>
using has_BindGetSocketNumber = std::integral_constant<bool, details::has_BindGetSocketNumber<T, Sig>::value>;

template<class T, class Sig>
using has_BindGetCurrentIndex = std::integral_constant<bool, details::has_BindGetCurrentIndex<T, Sig>::value>;

// enable_if Alias template
template<typename T>
using enable_if_has_BindSendPart = typename std::enable_if<has_BindSendPart<T,void(int)>::value,int>::type;
template<typename T>
using enable_if_hasNot_BindSendPart = typename std::enable_if<!has_BindSendPart<T,void(int)>::value,int>::type;

template<typename T>
using enable_if_has_BindGetSocketNumber = typename std::enable_if<has_BindGetSocketNumber<T,int()>::value,int>::type;
template<typename T>
using enable_if_hasNot_BindGetSocketNumber = typename std::enable_if<!has_BindGetSocketNumber<T,int()>::value,int>::type;

template<typename T>
using enable_if_has_BindGetCurrentIndex = typename std::enable_if<has_BindGetCurrentIndex<T,int()>::value,int>::type;
template<typename T>
using enable_if_hasNot_BindGetCurrentIndex = typename std::enable_if<!has_BindGetCurrentIndex<T,int()>::value,int>::type;

} // namespace tools
} // namespace FairMQ

#endif
