#!/bin/sh -e

if [ -z "$PG_CONNECT_STRING" ];
then
    export PG_CONNECT_STRING=$1
fi

if [ -z "$PG_CONNECT_STRING" -a -n "$TOP_SRCDIR" ];
then
    . $TOP_SRCDIR/connection.mk
    export PG_CONNECT_STRING
fi

if [ -z "$PG_CONNECT_STRING" ];
then
    echo variable PG_CONNECT_STRING not found
    exit 1
fi

baseDir=`pwd`
for dir in `ls | LC_ALL="C" grep '^[0-9][A-Za-z].*' | sort`; do
    echo "entering $dir"
    cd $dir
    if [ -f ./install ]; then
        ./install $PG_CONNECT_STRING
    else
        $baseDir/definstall $PG_CONNECT_STRING
    fi
    touch ok
    cd ..
done

