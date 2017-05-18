/********************************************************************************
 *    Copyright (C) 2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_TOOLS_CPPSTL_H
#define FAIR_MQ_TOOLS_CPPSTL_H

#include <memory>

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

} /* namespace tools */
} /* namespace mq */
} /* namespace fair */

#endif /* FAIR_MQ_TOOLS_CPPSTL_H */
