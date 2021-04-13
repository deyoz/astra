#!/bin/bash

NUM_COPIES=$1

echo createdb.sh:
echo CONNECT_STRING=${CONNECT_STRING}
echo PG_CONNECT_STRING=${PG_CONNECT_STRING}
echo PG_CONNECT_STRING_ARX=${PG_CONNECT_STRING_ARX}


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
      ( cd src && ./nosir.tcl -html_to_db ../${oradir}/4load/html )
      checkresult html_to_db $?
    )
}

build_pg_database()
{
     pgdir=$1
     ( ( cd ${pgdir} && ./create_database.sh ${PG_CONNECT_STRING} )
       checkresult create_pg_db $?
     )
     
     other_pg_bases_dir=${pgdir}/bases_pg/
     arx_base_dir=${other_pg_bases_dir}/arx
     
     # arx
     ( ( cd ${arx_base_dir} && ./../../create_database.sh ${PG_CONNECT_STRING_ARX} )
        checkresult create_arx_pg_db $?
     )
}

build_pg_database sql/bases/pg
checkresult build_pg_database $?

build_ora_database sql/bases/ora
checkresult build_ora_database $?


( cd src && make install-edimessages )
checkresult installedimessages $?

( cd src && ./nosir.tcl -load_fr ../sql/nosir_load/fr_reports )
checkresult load_fr $?

( cd src && ./nosir.tcl -pg_sessions_check )
checkresult pg_sessions_check $?
