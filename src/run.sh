#!/bin/sh
if [ -f ../locallibs/external_env_file ] ; then . ../locallibs/external_env_file ; fi
if [ $# -gt 0 ]
then
    LC_ALL=C \
    LD_LIBRARY_PATH=.:libs:airimp:${ORACLE_HOME}/lib:${BOOST}/lib:${LD_LIBRARY_PATH} \
    exec ${1+"$@"}
fi
