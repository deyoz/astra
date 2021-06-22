#!/bin/sh

echo 'Copy init config files in run directory...'
cp ./docker/RUN_EXAMPLE/* /opt/astra/run
echo 'OK'

echo 'setting connect string from ENV'
echo "set PG_CONNECT_STRING \"$PG_CONNECT_STRING\"
set PG_CONNECT_STRING_ARX \"$PG_CONNECT_STRING_ARX\"" | tee /opt/astra/run/local_after.tcl

echo 'OK'
