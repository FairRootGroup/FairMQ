/********************************************************************************
 *    Copyright (C) 2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "PMIx.h"
#include <FairMQLogger.h>
#include <fairmq/Tools.h>
#include <stdexcept>

namespace fair
{
namespace mq
{
namespace plugins
{

PMIx::PMIx(const std::string& name,
           const Plugin::Version version,
           const std::string& maintainer,
           const std::string& homepage,
           PluginServices* pluginServices)
    : Plugin(name, version, maintainer, homepage, pluginServices)
    , fPid(getpid())
{
    auto rc = PMIx_Init(&fPMIxProc, NULL, 0);
    if (rc != PMIX_SUCCESS) {
        throw std::runtime_error(tools::ToString("Client ns ", fPMIxProc.nspace,
                                          " rank ", fPMIxProc.rank,
                                          " pid ", fPid,
                                          ": PMIx_Init failed: ", rc));
    }

    LOG(info) << "Client ns " << fPMIxProc.nspace << " rank " << fPMIxProc.rank << " pid " << fPid
              << ": Running";
}

PMIx::~PMIx()
{
    LOG(info) << "Client ns " << fPMIxProc.nspace << " rank " << fPMIxProc.rank << " pid " << fPid
              << ": Finalizing";

    auto rc = PMIx_Finalize(NULL, 0);
    if (rc != PMIX_SUCCESS) {
        throw std::runtime_error(tools::ToString("Client ns ", fPMIxProc.nspace,
                                          " rank ", fPMIxProc.rank,
                                          " pid ", fPid,
                                          ": PMIx_Finalize failed: ", rc));
    }

    LOG(info) << "Client ns " << fPMIxProc.nspace << " rank " << fPMIxProc.rank << " pid " << fPid
              << ": PMIx_Finalize successfully completed";
}

} /* namespace plugins */
} /* namespace mq */
} /* namespace fair */
