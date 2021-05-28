################################################################################
#    Copyright (C) 2021 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    #
#                                                                              #
#              This software is distributed under the terms of the             #
#              GNU Lesser General Public Licence (LGPL) version 3,             #
#                  copied verbatim in the file "LICENSE"                       #
################################################################################

include_guard(GLOBAL)

include(FairMQBundlePackageHelper)

set(PROJECT_FairCMakeModules_VERSION 0.2)
find_package(FairCMakeModules ${PROJECT_FairCMakeModules_VERSION} QUIET)

if(NOT FairCMakeModules_FOUND AND NOT FairCMakeModules_BUNDLED)
  build_bundled(FairCMakeModules extern/FairCMakeModules)
  find_package(FairCMakeModules REQUIRED)
endif()
