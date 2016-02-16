#!/bin/bash -e

function uab_check_version() {
    v=`grep 'CHECK_M...._VERSION' $1/include/check.h | cut -f3 -d\  | tr '\n' ' '`
    test "$v" == "(0) (10) (0) "
}

function uab_config_and_build() {
    prefix=${1:?prefix as the 1st parameter}
    shift 1
    if [ -n "$PLATFORM" ] ; then
        export CXX="$LOCALCXX -$PLATFORM"
        export CC="$LOCALCC -$PLATFORM"
    else
        export CXX=$LOCALCXX
        export CC=$LOCALCC
    fi
    autoreconf --force --install
    ./configure --prefix=$prefix $@
    make -j${MAKE_J:-3}
    make install
}

function uab_pkg_tarball() {
    echo check-0.10.0.tar.gz
}

