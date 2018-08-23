#!/bin/bash -e
. ./env_file
[[ -n "$LOAD_A" ]] && load_a="-l$LOAD_A"

for dir in $@; do
    make -r -s -j$MAKE_J $load_a -C$dir  || exit 1
done
