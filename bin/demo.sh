#!/bin/bash
set -euo pipefail

(./buildFromScratch.sh no/ora --createtcl --createdb && cd src && ./nosir.tcl -tscript 0 ts/season_ldr.ts)
