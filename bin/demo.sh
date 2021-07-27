#!/bin/bash
# set -euo pipefail

checkresult()
{
    if [ $2 -ne 0 ]; then
        echo "$1 failed $2"
        exit 1
    fi
}

(./buildFromScratch.sh no/ora --createtcl --createdb && cd src && ./nosir.tcl -tscript 0 ts/season_ldr.ts)

checkresult load_season $?
