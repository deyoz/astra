#!/bin/bash -e
if [ -f $EXTLIB_ENV_FILE ]; then . $EXTLIB_ENV_FILE; fi

cd $(dirname $0)
./update_and_build.sh $@
