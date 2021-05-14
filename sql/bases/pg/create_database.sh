#!/bin/sh -e

if [ "$#" -ne 1 ]; then
    echo CONNECT_STRING for PG database not found
fi

baseDir=`pwd`
for dir in `ls | LC_ALL="C" grep '^[0-9][A-Za-z].*' | sort`; do
    echo "entering $dir"
    cd $dir
    if [ -f ./install ]; then
        ./install $1
    else
        $baseDir/definstall $1
    fi
    touch ok
    cd ..
done

