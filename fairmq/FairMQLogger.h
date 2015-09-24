/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQLogger.h
 *
 * @since 2012-12-04
 * @author D. Klein, A. Rybalchenko
 */

#ifndef FAIRMQLOGGER_H_
#define FAIRMQLOGGER_H_

#include <boost/version.hpp>

#if BOOST_VERSION < 105600
#include "logger/logger_oldboost_version.h"
#else
#include "logger/logger.h"
#endif 
typedef unsigned long long timestamp_t;

timestamp_t get_timestamp();

#endif /* FAIRMQLOGGER_H_ */
