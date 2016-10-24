#!/bin/bash -e

function uab_check_version() {
    grep -w '^[ ]*#[ ]*define[ ]\+LIBXSLT_VERSION[ ]\+10129' $1/include/libxslt/xsltconfig.h &>/dev/null
}

function uab_config_and_build() {
    prefix=${1:?prefix as the 1st parameter}
    shift 1
    #
    libxml_prefix=$(pkg-config --libs-only-L libxml-2.0) 
    libxml_prefix=${libxml_prefix#-L}
    libxml_prefix=${libxml_prefix%/*}
    #
    autoreconf --force --install
    ./configure --disable-maintainer-mode --with-libxml-prefix=$libxml_prefix --with-mem-debug --without-html --without-python --prefix=$prefix $@
    make -j${MAKE_J:-3}
    make install
}

function uab_pkg_tarball() {
    echo libxslt-1.1.29.tar.gz
}

