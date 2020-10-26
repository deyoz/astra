#!/bin/bash -e

uab_check_version() {
    v=`grep 'CHECK_M...._VERSION' $1/include/check.h | cut -f3 -d\  | tr '\n' ' '`
    test "$v" == "(0) (12) (0) "
}

uab_config_and_build() {
    prefix=${1:?prefix as the 1st parameter}
    shift 1
    autoreconf --force --install
    ./configure --prefix=$prefix "$@"
    make -j${MAKE_J:-3}
    make install
}

uab_pkg_tarball() {
    echo check-0.12.0.tar.gz
}

