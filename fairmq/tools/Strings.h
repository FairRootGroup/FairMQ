/********************************************************************************
 *    Copyright (C) 2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_TOOLS_STRINGS_H
#define FAIR_MQ_TOOLS_STRINGS_H

#include <array>
#include <initializer_list>
#include <sstream>
#include <string>
#include <vector>

namespace fair
{
namespace mq
{
namespace tools
{

/// @brief concatenates a variable number of args with the << operator via a stringstream
/// @param t objects to be concatenated
/// @return concatenated string
template<typename ... T>
auto ToString(T&&... t) -> std::string
{
    std::stringstream ss;
    (void)std::initializer_list<int>{(ss << t, 0)...};
    return ss.str();
}

/// @brief convert command line arguments from main function to vector of strings
inline auto ToStrVector(const int argc, char*const* argv, const bool dropProgramName = true) -> std::vector<std::string>
{
    if (dropProgramName) {
        return std::vector<std::string>(argv + 1, argv + argc);
    } else {
        return std::vector<std::string>(argv, argv + argc);
    }
}

} /* namespace tools */
} /* namespace mq */
} /* namespace fair */

#endif /* FAIR_MQ_TOOLS_STRINGS_H */
