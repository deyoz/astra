#!/bin/bash -e

function uab_check_version() {
    grep -w '^[ ]*#[ ]*define[ ]\+LIBXML_VERSION[ ]\+20904' $1/include/libxml2/libxml/xmlversion.h &>/dev/null
}

function uab_config_and_build() {
    prefix=${1:?prefix as the 1st parameter}
    shift 1
    autoreconf --force --install
    ./configure --disable-maintainer-mode --with-mem-debug --without-python --prefix=$prefix $@
    make -j${MAKE_J:-3}
    make install
}

function uab_pkg_tarball() {
    echo libxml2-2.9.4.tar.gz
}

