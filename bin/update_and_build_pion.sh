#!/bin/bash -e

function uab_check_version() {
    test "5.0.6" == "5.0.6"
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
    
    if [ "$CPP_STD_VERSION" = "c++11" ]; then
    	cxxflags="-std=c++11"
    elif [ "$CPP_STD_VERSION" = "c++14" ]; then
    	cxxflags="-std=c++14"
    fi

    ./autogen.sh
    ./configure --with-boost=$BOOST --prefix=$prefix CXXFLAGS=$cxxflags $@
    make -j${MAKE_J:-3}
    make install
}

function uab_pkg_tarball() {
    echo pion-5.0.6.tar.bz2
}

