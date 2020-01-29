#!/bin/bash -e

if [ -f ../sirenalibs/external_env_file ] ; then . ../sirenalibs/external_env_file ; fi
if [ $# -gt 0 ]
then
    LC_ALL=C \
    LD_LIBRARY_PATH=.:${ORACLE_HOME}/lib:$BOOST/lib:${LD_LIBRARY_PATH} \
    exec ${1+"$@"}
fi
