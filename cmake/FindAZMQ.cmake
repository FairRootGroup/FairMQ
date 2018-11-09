################################################################################
#    Copyright (C) 2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    #
#                                                                              #
#              This software is distributed under the terms of the             #
#              GNU Lesser General Public Licence (LGPL) version 3,             #
#                  copied verbatim in the file "LICENSE"                       #
################################################################################
#
# ###########################
# # Locate the AZMQ library #
# ###########################
#
#
# Usage:
#
#   find_package(AZMQ [version] [QUIET] [REQUIRED])
#
#
# Defines the following variables:
#
#   AZMQ_FOUND - Found the ZeroMQ library
#   AZMQ_INCLUDE_DIR (CMake cache) - Include directory
#
#
# Accepts the following variables as hints for installation directories:
#
#   AZMQ_ROOT (CMake var, ENV var)
#
#
# If the above variables are not defined, or if ZeroMQ could not be found there,
# it will look for it in the system directories. Custom ZeroMQ installations
# will always have priority over system ones.
#

if(NOT AZMQ_ROOT)
  set(AZMQ_ROOT $ENV{AZMQ_ROOT})
endif()

find_path(AZMQ_INCLUDE_DIR
  NAMES azmq/socket.hpp
  HINTS ${AZMQ_ROOT} $ENV{AZMQ_ROOT}
  PATH_SUFFIXES include
  DOC "AZMQ include directory"
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(AZMQ
    REQUIRED_VARS AZMQ_INCLUDE_DIR
)

if(AZMQ_FOUND AND NOT TARGET AZMQ::AZMQ)
  add_library(AZMQ::AZMQ INTERFACE IMPORTED)
  set_target_properties(AZMQ::AZMQ PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES ${AZMQ_INCLUDE_DIR}
    INTERFACE_LINK_LIBRARIES "libzmq;Boost::boost;Boost::container;Boost::system"
  )
endif()
