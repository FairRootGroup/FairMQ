/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQConfigurable.cxx
 *
 * @since 2012-10-25
 * @author D. Klein, A. Rybalchenko
 */

#include "FairMQLogger.h"
#include "FairMQConfigurable.h"

using namespace std;

FairMQConfigurable::FairMQConfigurable()
{
}

void FairMQConfigurable::SetProperty(const int key, const string& value)
{
    LOG(ERROR) << "Reached end of the property list. SetProperty(" << key << ", " << value << ") has no effect.";
    exit(EXIT_FAILURE);
}

string FairMQConfigurable::GetProperty(const int key, const string& default_ /*= ""*/)
{
    LOG(ERROR) << "Reached end of the property list. The requested property " << key << " was not found.";
    return default_;
}

void FairMQConfigurable::SetProperty(const int key, const int value)
{
    LOG(ERROR) << "Reached end of the property list. SetProperty(" << key << ", " << value << ") has no effect.";
    exit(EXIT_FAILURE);
}

int FairMQConfigurable::GetProperty(const int key, const int default_ /*= 0*/)
{
    LOG(ERROR) << "Reached end of the property list. The requested property " << key << " was not found.";
    return default_;
}

FairMQConfigurable::~FairMQConfigurable()
{
}
