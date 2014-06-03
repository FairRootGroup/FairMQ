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

#include "FairMQConfigurable.h"

FairMQConfigurable::FairMQConfigurable()
{
}

void FairMQConfigurable::SetProperty(const int key, const string& value, const int slot /*= 0*/)
{
}

string FairMQConfigurable::GetProperty(const int key, const string& default_ /*= ""*/, const int slot /*= 0*/)
{
    return default_;
}

void FairMQConfigurable::SetProperty(const int key, const int value, const int slot /*= 0*/)
{
}

int FairMQConfigurable::GetProperty(const int key, const int default_ /*= 0*/, const int slot /*= 0*/)
{
    return default_;
}

FairMQConfigurable::~FairMQConfigurable()
{
}
