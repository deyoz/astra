#!/bin/bash

NUM_COPIES=$1

echo createdb.sh:
echo CONNECT_STRING=${CONNECT_STRING}
echo PG_CONNECT_STRING=${PG_CONNECT_STRING}


checkresult()
{
    if [ $2 -ne 0 ]; then
        echo "$1 failed $2"
        exit 1
    fi
}

build_ora_database()
{
    oradir=$1
    ( ( cd ${oradir} && ./create_database.sh ${CONNECT_STRING} )
      checkresult create_ora_db $?
      ( cd src && make install-edimessages )
      checkresult installedimessages $?
    )
}

build_pg_database()
{
     pgdir=$1
     ( ( cd ${pgdir} && ./create_database.sh ${PG_CONNECT_STRING} )
       checkresult create_pg_db $?
     )
}

build_pg_database sql/bases/pg
checkresult build_pg_database $?

build_ora_database sql/bases/ora
checkresult build_ora_database $?
