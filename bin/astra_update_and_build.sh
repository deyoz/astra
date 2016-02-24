#!/bin/bash -e
if [ -f locallibs/external_env_file ]; then . locallibs/external_env_file; fi

cd $(dirname $0)
./update_and_build.sh $@
