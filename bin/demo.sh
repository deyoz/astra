#!/bin/bash
set -euo pipefail

(cd src && ./nosir.tcl -tscript 0 ts/season_ldr.ts)
