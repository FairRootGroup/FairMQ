/********************************************************************************
 *    Copyright (C) 2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_TOOLS_VERSION_H
#define FAIR_MQ_TOOLS_VERSION_H

#include <ostream>
#include <tuple>

namespace fair
{
namespace mq
{
namespace tools
{

struct Version
{
    const int fkMajor, fkMinor, fkPatch;

    friend auto operator< (const Version& lhs, const Version& rhs) -> bool { return std::tie(lhs.fkMajor, lhs.fkMinor, lhs.fkPatch) < std::tie(rhs.fkMajor, rhs.fkMinor, rhs.fkPatch); }
    friend auto operator> (const Version& lhs, const Version& rhs) -> bool { return rhs < lhs; }
    friend auto operator<=(const Version& lhs, const Version& rhs) -> bool { return !(lhs > rhs); }
    friend auto operator>=(const Version& lhs, const Version& rhs) -> bool { return !(lhs < rhs); }
    friend auto operator==(const Version& lhs, const Version& rhs) -> bool { return std::tie(lhs.fkMajor, lhs.fkMinor, lhs.fkPatch) == std::tie(rhs.fkMajor, rhs.fkMinor, rhs.fkPatch); }
    friend auto operator!=(const Version& lhs, const Version& rhs) -> bool { return !(lhs == rhs); }
    friend auto operator<<(std::ostream& os, const Version& v) -> std::ostream& { return os << v.fkMajor << "." << v.fkMinor << "." << v.fkPatch; }
};

} /* namespace tools */
} /* namespace mq */
} /* namespace fair */

#endif /* FAIR_MQ_TOOLS_VERSION_H */
