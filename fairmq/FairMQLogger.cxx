/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *              GNU Lesser General Public Licence (LGPL) version 3,             *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQLogger.cxx
 *
 * @since 2012-12-04
 * @author D. Klein, A. Rybalchenko
 */

#include "FairMQLogger.h"

#include <sys/time.h>
#include <ctime>

timestamp_t get_timestamp()
{
    struct timeval now;
    gettimeofday(&now, NULL);
    return now.tv_usec + static_cast<timestamp_t>(now.tv_sec) * 1000000;
}
