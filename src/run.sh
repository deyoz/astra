#!/bin/sh

if [ $# -gt 0 ]
then
    LC_ALL=C \
    LD_LIBRARY_PATH=.:libs:airimp:${ORACLE_HOME}/lib:${LD_LIBRARY_PATH} \
    exec ${1+"$@"}
fi
