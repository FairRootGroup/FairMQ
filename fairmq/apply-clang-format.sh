#!/bin/bash

find . -type f \( -iname "*.h" ! -iname "*.pb.h" -o -iname "*.cxx" \) -execdir clang-format -i {} \;
