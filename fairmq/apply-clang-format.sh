#!/bin/bash

find . -type f \( -iname "*.h" ! -iname "*.pb.h" ! -iname "*LinkDef.h" -o -iname "*.cxx" -o -iname "*.tpl" \) -execdir clang-format -i {} \;
