/********************************************************************************
 * Copyright (C) 2014-2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
#ifndef FAIR_MQ_PROPERTYOUT_H
#define FAIR_MQ_PROPERTYOUT_H

#include <fairmq/Properties.h>

#include <sstream>

inline std::ostream& operator<<(std::ostream& os, const fair::mq::Property& p)
{
    return os << fair::mq::PropertyHelper::GetPropertyInfo(p).first;
}

#endif /* FAIR_MQ_PROPERTYOUT_H */
