#!/bin/sh -e

if [ -z "$CONNECT_STRING" ];
then
    export CONNECT_STRING=$1
fi

if [ -z "$CONNECT_STRING" -a -n "$TOP_SRCDIR" ];
then
    . $TOP_SRCDIR/connection.mk
    export CONNECT_STRING
fi

if [ -z "$CONNECT_STRING" ];
then
    echo variable CONNECT_STRING not found
    exit 1
fi

baseDir=`pwd`
for dir in `ls | LC_ALL="C" grep '^[0-9][A-Za-z].*' | sort`; do
    echo "entering $dir"
    cd $dir
    $baseDir/definstall $CONNECT_STRING
    touch ok
    cd ..
done

