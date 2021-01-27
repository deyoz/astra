#!/bin/bash

# Functions --------------------------------------------------------------------

function prepare_dirs()
{
  for dir in "$@" ; do
    mkdir -p "$dir"
    rm -f "$dir"/*.sql
  done
}
export -f prepare_dirs

# Script -----------------------------------------------------------------------

# Check parameters
if [ -z "$1" ]; then
  echo "Usage: $(basename "$0") <connect_string>"
  exit 1
fi

# Check for dirs not exists
if [ ! -d "1Tab" ] && [ ! -d "1Tab_other" ] && [ "$2" != "--force" ] ; then
  echo "SQL directories (1Tab, ...) were not found. Check pwd"
  echo "(to create new: $(basename "$0") <connect_string> --force)"
  exit 1
fi

# Dump
if echo "$1" | grep -P "postgres(ql)?://" &>/dev/null ; then
  echo "PostgreSQL"
  SCRIPTS="$(dirname "$0")/pgsql_bin"
else
  echo "Oracle"
  SCRIPTS="$(dirname "$0")/oracle_bin"
fi
export readonly SCRIPTS
. "$SCRIPTS/dump.sh" "$1"
echo "TKyte DONE"
