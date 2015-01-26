#!/bin/bash

NUM_COPIES=$1

echo createdb.sh:
echo NUM_COPIES=${NUM_COPIES}
echo CONNECT_STRING=${CONNECT_STRING}


checkresult()
{
    if [ $2 -ne 0 ]; then
        echo "$1 failed $2"
        exit 1
    fi
}

build_database()
{
    sqldir=$1
    ( ( cd ${sqldir} && SYSPAROL=${SYSPAROL:-system/manager`echo $CONNECT_STRING | grep -o "@.*$"`} PATH=${ORACLE_HOME}/bin:${PATH} ./create_database.sh )
      checkresult createdb $?
      ( cd src && make install-edimessages )
      checkresult installedimessages $?
    )
}

pids=()
rm -rf sql_copy*.tmp
for i in $(seq 0 $(expr ${NUM_COPIES:-0} - 1)); do
    if [ "${i}" == "0" ]; then
        build_database sql &
    else
        connstr_copy=$(echo $CONNECT_STRING | sed "s@\([^/]\+\)@\1_copy${i}@")
        cp -a sql sql_copy${i}.tmp
        ( export CONNECT_STRING=$connstr_copy; build_database sql_copy${i}.tmp ) &
    fi
    pids[${i}]=$!
done
for i in $(seq 0 $(expr ${NUM_COPIES:-0} - 1)); do
    pid=${pids[${i}]}
    echo waiting for build_database${i} pid=${pid}
    wait ${pid}
    checkresult build_database${i} $?
    echo ${pid} done
done
