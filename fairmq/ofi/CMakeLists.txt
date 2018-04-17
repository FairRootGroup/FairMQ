################################################################################
#    Copyright (C) 2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    #
#                                                                              #
#              This software is distributed under the terms of the             #
#              GNU Lesser General Public Licence (LGPL) version 3,             #
#                  copied verbatim in the file "LICENSE"                       #
################################################################################

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS Control.proto)

add_library(OfiTransport OBJECT ${PROTO_SRCS} ${PROTO_HDRS})
target_include_directories(OfiTransport PRIVATE $<TARGET_PROPERTY:protobuf::libprotobuf,INTERFACE_INCLUDE_DIRECTORIES>)
