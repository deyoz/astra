#!/bin/bash
export AXIS2C_HOME=/usr/lib/axis2/
gcc -g -shared -fPIC -olibFFPServer.so -I $AXIS2C_HOME/include/axis2-1.6.0/ -Isrc -L$AXIS2C_HOME/lib \
    -laxutil \
    -laxis2_axiom \
    -laxis2_engine \
    -laxis2_parser \
    -lpthread \
    -laxis2_http_sender \
    -laxis2_http_receiver \
    -lguththila \
    *.c src/*.c
