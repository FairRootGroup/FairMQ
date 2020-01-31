#!/bin/bash

################################################################################
#    Copyright (C) 2019 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    #
#                                                                              #
#              This software is distributed under the terms of the             #
#              GNU Lesser General Public Licence (LGPL) version 3,             #
#                  copied verbatim in the file "LICENSE"                       #
################################################################################

export PATH=@BIN_DIR@:$PATH

OS=$(uname -s 2>&1)
if [ "$OS" == "Darwin" ]; then
   export DYLD_LIBRARY_PATH=@LIB_DIR@:$DYLD_LIBRARY_PATH
fi
