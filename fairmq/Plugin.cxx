/********************************************************************************
 *    Copyright (C) 2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <fairmq/Plugin.h>
#include <FairMQLogger.h>

using namespace std;

fair::mq::Plugin::Plugin(const string name, const Version version, const string maintainer, const string homepage)
: fkName(name)
, fkVersion(version)
, fkMaintainer(maintainer)
, fkHomepage(homepage)
{
	LOG(DEBUG) << "Loaded plugin: " << *this;
}

fair::mq::Plugin::~Plugin()
{
	LOG(DEBUG) << "Unloaded plugin: " << *this;
}
