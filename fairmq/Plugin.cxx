/********************************************************************************
 * Copyright (C) 2017-2019 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <fairmq/Plugin.h>
#include <fairlogger/Logger.h>
#include <utility>

using namespace std;

fair::mq::Plugin::Plugin(string name,
                         Version version,
                         string maintainer,
                         string homepage,
                         PluginServices* pluginServices)
    : fkName(std::move(name))
    , fkVersion(version)
    , fkMaintainer(std::move(maintainer))
    , fkHomepage(std::move(homepage))
    , fPluginServices(pluginServices)
{
    LOG(debug) << "Loaded plugin: " << *this;
}

fair::mq::Plugin::~Plugin()
{
    LOG(debug) << "Unloaded plugin: " << *this;
}
