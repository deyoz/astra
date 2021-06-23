#!/bin/sh

echo "Updating executables & libs & demo_local_after.tcl ..."
cp /opt/astra/src/astra /opt/astra/run \
    && cp /opt/astra/locallibs/serverlib/src/dispatcher /opt/astra/run \
    && cp /opt/astra/locallibs/serverlib/src/supervisor /opt/astra/run \
    && cp /opt/astra/locallibs/serverlib/*tcl /opt/astra/run \
    && cp /opt/astra/src/demo_local_after.tcl /opt/astra/run \
    && mkdir /opt/astra/run/libs && find ./externallibs -maxdepth 3 -name '*.so*' -exec cp '{}' /opt/astra/run/libs/ \;
echo "OK"

echo "Starting astra!"
date

cd /opt/astra/run && ./run
