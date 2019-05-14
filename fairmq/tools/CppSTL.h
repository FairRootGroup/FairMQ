/********************************************************************************
 *    Copyright (C) 2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_TOOLS_CPPSTL_H
#define FAIR_MQ_TOOLS_CPPSTL_H

#include <functional>
#include <memory>
#include <type_traits>

namespace fair
{
namespace mq
{
namespace tools
{

// make_unique implementation, until C++14 is default
template<typename T, typename ...Args>
std::unique_ptr<T> make_unique(Args&& ...args)
{
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

// make_unique implementation (array variant), until C++14 is default
template<typename T>
std::unique_ptr<T> make_unique(std::size_t size)
{
    return std::unique_ptr<T>(new typename std::remove_extent<T>::type[size]());
}

// provide an enum hasher to compensate std::hash not supporting enums in C++11
template<typename Enum>
struct HashEnum
{
    auto operator()(const Enum& e) const noexcept
    -> typename std::enable_if<std::is_enum<Enum>::value, std::size_t>::type
    {
        using _type = typename std::underlying_type<Enum>::type;
        return std::hash<_type>{}(static_cast<_type>(e));
    }
};

} /* namespace tools */
} /* namespace mq */
} /* namespace fair */

#endif /* FAIR_MQ_TOOLS_CPPSTL_H */
