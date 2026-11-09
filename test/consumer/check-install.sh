#!/bin/bash

################################################################################
# Copyright (C) 2026 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH       #
#                                                                              #
#              This software is distributed under the terms of the             #
#              GNU Lesser General Public Licence (LGPL) version 3,             #
#                  copied verbatim in the file "LICENSE"                       #
################################################################################

# Checks an installed FairMQ:
#   1. install to a temporary prefix that differs from CMAKE_INSTALL_PREFIX
#   2. check that no installed file refers to the configure-time prefix
#      (scripts must be configured for the prefix given at install time)
#   3. move the installation to a second prefix and build and run a small
#      downstream project against it (the CMake package must be relocatable)
#
# Usage: check-install.sh <fairmq-build-dir> [extra cmake configure args...]
#
# Dependencies of FairMQ (FairLogger, FairCMakeModules, ...) must be found via
# CMAKE_PREFIX_PATH (environment or extra cmake args).

set -euo pipefail

if [ $# -lt 1 ]; then
  echo "Usage: $0 <fairmq-build-dir> [extra cmake configure args...]" >&2
  exit 2
fi

here=$(cd "$(dirname "$0")" && pwd)
build=$(cd "$1" && pwd)
shift

work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
first="$work/prefix-one"
second="$work/prefix-two"

echo "--- install to $first"
cmake --install "$build" --prefix "$first" > "$work/install.log"

echo "--- check for references to the configure-time prefix"
configured=$(sed -n 's/^CMAKE_INSTALL_PREFIX:PATH=//p' "$build/CMakeCache.txt")
if [ -n "$configured" ] && [ "$configured" != "$first" ] && grep -rIl -F "$configured/" "$first"; then
  echo "ERROR: installed files above refer to the configure-time prefix $configured" >&2
  exit 1
fi

echo "--- relocate to $second"
mv "$first" "$second"

echo "--- build downstream project"
cmake -S "$here" -B "$work/consumer" -DCMAKE_PREFIX_PATH="$second${CMAKE_PREFIX_PATH:+;$CMAKE_PREFIX_PATH}" "$@"
cmake --build "$work/consumer"
LD_LIBRARY_PATH="$second/lib:$second/lib64${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" "$work/consumer/consumer"
echo "OK"
