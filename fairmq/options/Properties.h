/********************************************************************************
 * Copyright (C) 2014-2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
#ifndef FAIR_MQ_PROPERTIES_H
#define FAIR_MQ_PROPERTIES_H

#include <boost/any.hpp>

#include <map>
#include <string>

namespace fair
{
namespace mq
{

using Property = boost::any;

using Properties = std::map<std::string, Property>;

}
}

#endif /* FAIR_MQ_PROPERTIES_H */
