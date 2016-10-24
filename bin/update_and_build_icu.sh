#!/bin/bash -e

function uab_check_version() {
    grep -w 'U_ICU_VERSION[ ]\+\"57.1\"' $1/include/unicode/uvernum.h
}

function uab_config_and_build() {
    prefix=${1:?prefix as the 1st parameter}
    shift 1
    
    [ -z "$CPP_STD_VERSION" ] && die "CPP_STD_VERSION not set"
    cxxflags="-std=$CPP_STD_VERSION"

    cd $prefix/src/source
    ./configure --prefix=$prefix CXXFLAGS=$cxxflags $@
    make -j${MAKE_J:-3}
    make install
}

function uab_pkg_tarball() {
    echo icu4c-57_1.tar.gz
}

