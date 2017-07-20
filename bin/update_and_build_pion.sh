#!/bin/bash -e

function uab_check_version() {
    grep -w 'PION_VERSION[ ]\+\"5.0.6\"' $1/include/pion/config.hpp
}

function uab_config_and_build() {
    prefix=${1:?prefix as the 1st parameter}
    shift 1

    ./autogen.sh
    ./configure --with-boost=$BOOST --prefix=$prefix CXXFLAGS="$CXXFLAGS" LDFLAGS="$LDFLAGS" $@
    make -j${MAKE_J:-3}
    make install
}

function uab_pkg_tarball() {
    echo pion-5.0.6.tar.bz2
}

