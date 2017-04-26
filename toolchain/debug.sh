#!/bin/bash

source ./env

echo "path: $(tch path)"
echo "cc: $(tch compiler)"
echo "cxx: $(tch compiler c++)"
echo "cflags: $(tch cflags)"
echo "include-flags: $(tch include-flags)"
echo "include-dirs: $(tch include-dirs)"
echo "lib-flags: $(tch lib-flags)"
echo "lib-dirs: $(tch lib-dirs)"
echo "dynamic-linker: $(tch dynamic-linker)"
echo "rpath: $(tch rpath)"

