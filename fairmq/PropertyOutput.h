/********************************************************************************
 *    Copyright (C) 2019 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
#ifndef FAIR_MQ_PROPERTYOUT_H
#define FAIR_MQ_PROPERTYOUT_H

#include <fairmq/Properties.h>

namespace boost
{

inline std::ostream& operator<<(std::ostream& os, const boost::any& p)
{
    return os << fair::mq::PropertyHelper::GetPropertyInfo(p).first;
}

}

#endif /* FAIR_MQ_PROPERTYOUT_H */
