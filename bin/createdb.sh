#!/bin/bash
set -euo pipefail
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
    set +e
    if [ "$ENABLE_ORACLE" = "0" ]; then
      echo "skip building ORA DB. ENABLE_ORACLE=$ENABLE_ORACLE"
    else
        oradir=$1
        ( cd ${oradir} && ./create_database.sh ${CONNECT_STRING} )
        checkresult create_ora_db $?
    fi
}

build_pg_database()
{
    set +e
    pgdir=$1
    ( cd ${pgdir} && ./create_database.sh ${PG_CONNECT_STRING} )
    checkresult create_pg_db $?

    other_pg_bases_dir=${pgdir}/bases_pg/
    arx_base_dir=${other_pg_bases_dir}/arx

     # arx
     ( cd ${arx_base_dir} && ./../../create_database.sh ${PG_CONNECT_STRING_ARX} )
     checkresult create_arx_pg_db $?
 }

build_pg_database sql/bases/pg
checkresult build_pg_database $?

build_ora_database sql/bases/ora
checkresult build_ora_database $?

( cd src && make install-edimessages )
checkresult installedimessages $?

( cd src && ./nosir.tcl -html_to_db ../sql/nosir_load/html )
checkresult html_to_db $?

( cd src && ./nosir.tcl -load_fr ../sql/nosir_load/fr_reports )
checkresult load_fr $?

( cd src && ./nosir.tcl -comp_elem_types_to_db ../sql/nosir_load/comp_elem_types.dat )
checkresult comp_elem_types_to_db $?

( cd src && ./nosir.tcl -pg_sessions_check )
checkresult pg_sessions_check $?

#( cd src && ./nosir.tcl -tscript 0 ts/season_ldr.ts )
#checkresult season_ldr $?

if [ "$ENABLE_ORACLE" = "0" ]; then
      echo "==== ENABLE_ORACLE=$ENABLE_ORACLE ORACLE DB not built ====="
fi
